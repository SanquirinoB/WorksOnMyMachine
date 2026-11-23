#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "UObject/UObjectGlobals.h"

#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "CoreKitNavigation/CoreKitMenuWidget.h"
#include "CoreKitNavigation/CoreKitNavigationGlyphs.h"
#include "CoreKitNavigation/CoreKitNavigationSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/InputSettings.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"

class FCoreKitNavigationInput final : public IInputProcessor
{
public:
	explicit FCoreKitNavigationInput(UCoreKitNavigationSubsystem* InOwner) : Owner(InOwner) {}

	void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}

	bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
	{
		const FKey Key = Event.GetKey();
		if (!Key.IsValid())
		{
			return false;
		}
		if (!Event.IsRepeat())
		{
			Notify(Event, Key.IsGamepadKey() ? ECoreKitInputDevice::Gamepad : ECoreKitInputDevice::Keyboard);
			if (Capture(Event, Key))
			{
				return true;
			}
		}
		if (UCoreKitNavigationSubsystem* Subsystem = Owner.Get())
		{
			return Subsystem->HandleMenuKey(Event.GetUserIndex(), Key, Event.IsRepeat());
		}
		return false;
	}

	bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent& Event) override
	{
		if (Event.GetKey().IsGamepadKey() && FMath::Abs(Event.GetAnalogValue()) >= 0.35f)
		{
			Notify(Event, ECoreKitInputDevice::Gamepad);
		}
		return false;
	}

	bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent& Event) override
	{
		if (!Event.IsTouchEvent() && !Event.GetCursorDelta().IsNearlyZero())
		{
			Notify(Event, ECoreKitInputDevice::Mouse);
		}
		return false;
	}

	bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent& Event) override
	{
		if (!Event.IsTouchEvent())
		{
			Notify(Event, ECoreKitInputDevice::Mouse);
			return Capture(Event, Event.GetEffectingButton());
		}
		return false;
	}

	bool HandleMouseWheelOrGestureEvent(FSlateApplication&, const FPointerEvent& Event, const FPointerEvent*) override
	{
		if (!Event.IsTouchEvent() && !FMath::IsNearlyZero(Event.GetWheelDelta()))
		{
			Notify(Event, ECoreKitInputDevice::Mouse);
		}
		return false;
	}

private:
	bool Capture(const FInputEvent& Event, FKey Key) const
	{
		if (UCoreKitNavigationSubsystem* Subsystem = Owner.Get())
		{
			return Subsystem->CaptureKey(Event.GetUserIndex(), Key);
		}
		return false;
	}

	void Notify(const FInputEvent& Event, ECoreKitInputDevice Device) const
	{
		if (UCoreKitNavigationSubsystem* Subsystem = Owner.Get())
		{
			Subsystem->ObserveInput(Event.GetUserIndex(), Device);
		}
	}

	TWeakObjectPtr<UCoreKitNavigationSubsystem> Owner;
};

void UCoreKitNavigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCoreKitNavigationSubsystem::HandlePostLoadMap);
}

void UCoreKitNavigationSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// This delegate is global: another PIE/game instance must not clear this player's menus.
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LoadedWorld || !LocalPlayer || LoadedWorld->GetGameInstance() != LocalPlayer->GetGameInstance())
	{
		return;
	}

	// The local player (and this subsystem) survives travel, the menus of the previous world do not. Drop them, and
	// if a menu had switched the viewport to UI input, hand game input to the new controller: otherwise the viewport
	// keeps UI-only mode without focus and the new level receives no input (seen on Retry from the defeat screen).
	Menus.RemoveAll([LoadedWorld](const FMenuEntry& Entry) { return !Entry.Menu.IsValid() || Entry.Menu->GetWorld() != LoadedWorld; });
	if (UCoreKitMenuWidget* Previous = ActiveMenu.Get(); Previous && Previous->GetWorld() != LoadedWorld)
	{
		ActiveMenu.Reset();
	}
	FocusScopes.RemoveAll([LoadedWorld](const FMenuFocusScope& Scope)
	{
		UUserWidget* Menu = Scope.Menu.Get();
		if (Menu && Menu->GetWorld() == LoadedWorld)
		{
			return false;
		}
		if (Menu)
		{
			Menu->OnNativeDestruct.Remove(Scope.OnDestruct);
		}
		return true;
	});
	CancelKeyCapture();
	RefreshTopMenu();
}

void UCoreKitNavigationSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	SetDeviceTrackingEnabled(false);
	for (const FMenuFocusScope& Scope : FocusScopes)
	{
		if (UUserWidget* Menu = Scope.Menu.Get())
		{
			Menu->OnNativeDestruct.Remove(Scope.OnDestruct);
		}
	}
	FocusScopes.Reset();
	Menus.Reset();
	ActiveMenu.Reset();
	Super::Deinitialize();
}

void UCoreKitNavigationSubsystem::SetDeviceTrackingEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		if (!InputProcessor && FSlateApplication::IsInitialized())
		{
			const TSharedRef<FCoreKitNavigationInput> Processor = MakeShared<FCoreKitNavigationInput>(this);
			if (FSlateApplication::Get().RegisterInputPreProcessor(Processor))
			{
				InputProcessor = Processor;
			}
		}
		return;
	}

	if (InputProcessor && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();
	CancelKeyCapture();
}

bool UCoreKitNavigationSubsystem::BeginKeyCapture()
{
	if (bCapturingKey)
	{
		return false;
	}
	SetDeviceTrackingEnabled(true);
	bCapturingKey = InputProcessor.IsValid();
	return bCapturingKey;
}

bool UCoreKitNavigationSubsystem::CancelKeyCapture()
{
	if (!bCapturingKey)
	{
		return false;
	}
	bCapturingKey = false;
	OnKeyCaptureFinished.Broadcast(false, FKey());
	return true;
}

bool UCoreKitNavigationSubsystem::CaptureKey(uint32 SlateUserIndex, FKey Key)
{
	if (!bCapturingKey || !Key.IsValid())
	{
		return false;
	}
	const auto User = GetLocalPlayer()->GetSlateUser();
	if (!User || SlateUserIndex != static_cast<uint32>(User->GetUserIndex()))
	{
		return false;
	}
	if (Key == EKeys::Escape)
	{
		return CancelKeyCapture();
	}
	bCapturingKey = false;
	OnKeyCaptureFinished.Broadcast(true, Key);
	return true;
}

void UCoreKitNavigationSubsystem::ObserveInput(uint32 SlateUserIndex, ECoreKitInputDevice Device)
{
	const TSharedPtr<FSlateUser> User = GetLocalPlayer()->GetSlateUser();
	if (!User || SlateUserIndex != static_cast<uint32>(User->GetUserIndex()))
	{
		return;
	}
	if (Device == ECoreKitInputDevice::Gamepad)
	{
		RefreshGamepadType();
	}
	if (Device != InputDevice)
	{
		InputDevice = Device;
		if (bMenuInputApplied)
		{
			ApplyMenuInput();
		}
		OnInputDeviceChanged.Broadcast(Device);
	}
}

FName UCoreKitNavigationSubsystem::GetGamepadType() const
{
	if (!GamepadTypeOverride.IsNone())
	{
		return GamepadTypeOverride;
	}
	if (!DetectedGamepadType.IsNone())
	{
		return DetectedGamepadType;
	}
	return GetDefault<UCoreKitNavigationSettings>()->DefaultGamepadType;
}

void UCoreKitNavigationSubsystem::SetGamepadTypeOverride(FName GamepadType)
{
	const FName Previous = GetGamepadType();
	GamepadTypeOverride = GamepadType;
	if (GetGamepadType() != Previous)
	{
		OnGamepadTypeChanged.Broadcast(GetGamepadType());
	}
}

void UCoreKitNavigationSubsystem::SetDetectedGamepadType(FName GamepadType)
{
	const FName Previous = GetGamepadType();
	DetectedGamepadType = GamepadType;
	if (GetGamepadType() != Previous)
	{
		OnGamepadTypeChanged.Broadcast(GetGamepadType());
	}
}

void UCoreKitNavigationSubsystem::RefreshGamepadType()
{
	const UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get();
	const ULocalPlayer* Player = GetLocalPlayer();
	if (!Devices || !Player)
	{
		return;
	}
	const FHardwareDeviceIdentifier Hardware = Devices->GetMostRecentlyUsedHardwareDevice(Player->GetPlatformUserId());
	if (Hardware.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad)
	{
		SetDetectedGamepadType(GetDefault<UCoreKitNavigationSettings>()->ResolveGamepadType(Hardware));
	}
}

FCoreKitKeyPresentation UCoreKitNavigationSubsystem::FindKeyPresentation(FKey Key) const
{
	FCoreKitKeyPresentation Presentation;
	if (!Key.IsValid())
	{
		return Presentation;
	}
	const UCoreKitNavigationSettings* Settings = GetDefault<UCoreKitNavigationSettings>();
	if (const UCoreKitInputIconSet* Set = Settings->LoadIconSet(Key.IsGamepadKey(), GetGamepadType()))
	{
		if (Set->FindKey(Key, Presentation))
		{
			return Presentation;
		}
	}
	Presentation.DisplayName = Key.GetDisplayName(false);
	return Presentation;
}

bool UCoreKitNavigationSubsystem::FindActionPresentation(
	const UInputAction* Action, FKey& OutKey, FCoreKitKeyPresentation& OutPresentation) const
{
	OutKey = FKey();
	OutPresentation = FCoreKitKeyPresentation();
	const UEnhancedInputLocalPlayerSubsystem* EnhancedInput =
		GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!EnhancedInput || !IsValid(Action))
	{
		return false;
	}

	const bool bWantGamepad = InputDevice == ECoreKitInputDevice::Gamepad;
	FKey Fallback;
	for (const FKey& Key : EnhancedInput->QueryKeysMappedToAction(Action))
	{
		if (Key.IsGamepadKey() != bWantGamepad)
		{
			continue;
		}
		const FCoreKitKeyPresentation Presentation = FindKeyPresentation(Key);
		if (!Presentation.Icon.IsNull())
		{
			OutKey = Key;
			OutPresentation = Presentation;
			return true;
		}
		if (!Fallback.IsValid())
		{
			Fallback = Key;
		}
	}
	if (Fallback.IsValid())
	{
		OutKey = Fallback;
		OutPresentation = FindKeyPresentation(Fallback);
		return true;
	}
	return false;
}

bool UCoreKitNavigationSubsystem::PushMenuFocus(UWidget* InitialWidget)
{
	return PushMenuFocusScope(InitialWidget).IsValid();
}

FGuid UCoreKitNavigationSubsystem::PushMenuFocusScope(UWidget* InitialWidget, UUserWidget* Menu)
{
	if (!IsValid(InitialWidget) || !FSlateApplication::IsInitialized())
	{
		return {};
	}

	ULocalPlayer* Player = GetLocalPlayer();
	const TSharedPtr<FSlateUser> User = Player->GetSlateUser();
	const TSharedPtr<SWidget> Widget = InitialWidget->GetCachedWidget();
	if (!User || !Widget || (InitialWidget->GetOwningLocalPlayer() && InitialWidget->GetOwningLocalPlayer() != Player))
	{
		return {};
	}

	FSlateApplication& Slate = FSlateApplication::Get();
	const uint32 UserIndex = User->GetUserIndex();
	const FGuid Id = FGuid::NewGuid();
	FMenuFocusScope& Scope = FocusScopes.AddDefaulted_GetRef();
	Scope.Id = Id;
	Scope.Target = Widget;
	Scope.Previous = Slate.GetUserFocusedWidget(UserIndex);
	if (IsValid(Menu))
	{
		Scope.Menu = Menu;
		Scope.OnDestruct = Menu->OnNativeDestruct.AddUObject(this, &ThisClass::HandleMenuDestruct, Id);
	}
	if (!Slate.SetUserFocus(UserIndex, Widget))
	{
		ReleaseMenuFocusScope(Id);
		return {};
	}

	return Id;
}

void UCoreKitNavigationSubsystem::PopMenuFocus()
{
	if (!FocusScopes.IsEmpty())
	{
		ReleaseMenuFocusScope(FocusScopes.Last().Id);
	}
}

void UCoreKitNavigationSubsystem::HandleMenuDestruct(UUserWidget*, FGuid Id)
{
	ReleaseMenuFocusScope(Id);
}

bool UCoreKitNavigationSubsystem::ReleaseMenuFocusScope(FGuid Id)
{
	const int32 Index = FocusScopes.IndexOfByPredicate([Id](const FMenuFocusScope& Entry) { return Entry.Id == Id; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	const FMenuFocusScope Removed = FocusScopes[Index];
	FocusScopes.RemoveAt(Index);
	if (UUserWidget* Menu = Removed.Menu.Get())
	{
		Menu->OnNativeDestruct.Remove(Removed.OnDestruct);
	}
	if (FocusScopes.IsValidIndex(Index))
	{
		FocusScopes[Index].Previous = Removed.Previous;
		return true;
	}

	if (!FSlateApplication::IsInitialized())
	{
		return true;
	}

	const TSharedPtr<FSlateUser> User = GetLocalPlayer()->GetSlateUser();
	if (User)
	{
		FSlateApplication& Slate = FSlateApplication::Get();
		const uint32 UserIndex = User->GetUserIndex();
		if (const auto Previous = Removed.Previous.Pin(); Previous && Slate.SetUserFocus(UserIndex, Previous))
		{
			return true;
		}
		for (int32 ScopeIndex = FocusScopes.Num() - 1; ScopeIndex >= 0; --ScopeIndex)
		{
			if (const auto Target = FocusScopes[ScopeIndex].Target.Pin(); Target && Slate.SetUserFocus(UserIndex, Target))
			{
				return true;
			}
		}
		Slate.SetUserFocusToGameViewport(UserIndex);
	}
	return true;
}

UTexture2D* UCoreKitNavigationSubsystem::FindActionGlyph(
	const UInputAction* Action, const UCoreKitNavigationGlyphs* Glyphs) const
{
	const UEnhancedInputLocalPlayerSubsystem* EnhancedInput =
		GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!EnhancedInput || !IsValid(Action) || !IsValid(Glyphs))
	{
		return nullptr;
	}

	for (const FKey& Key : EnhancedInput->QueryKeysMappedToAction(Action))
	{
		if (Key.IsGamepadKey() == (InputDevice == ECoreKitInputDevice::Gamepad))
		{
			if (UTexture2D* Glyph = Glyphs->FindGlyph(Key))
			{
				return Glyph;
			}
		}
	}
	return nullptr;
}
