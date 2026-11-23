// Menu stack of UCoreKitNavigationSubsystem: layers, input mode, Return/sections and prompts.
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"

#include "CoreKitNavigation/CoreKitMenuWidget.h"
#include "CoreKitNavigation/CoreKitNavigationSettings.h"
#include "CoreKitNavigation/CoreKitPromptWidget.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

APlayerController* UCoreKitNavigationSubsystem::GetMenuPlayerController() const
{
	const ULocalPlayer* Player = GetLocalPlayer();
	return Player ? Player->GetPlayerController(Player->GetWorld()) : nullptr;
}

bool UCoreKitNavigationSubsystem::PushMenu(UCoreKitMenuWidget* Menu, FName Layer)
{
	if (!IsValid(Menu) || IsMenuOpen(Menu))
	{
		return false;
	}
	const UCoreKitNavigationSettings* Settings = GetDefault<UCoreKitNavigationSettings>();
	FMenuEntry& Entry = Menus.AddDefaulted_GetRef();
	Entry.Menu = Menu;
	Entry.Layer = Layer.IsNone() ? FName(TEXT("Menu")) : Layer;
	Entry.Priority = Settings->GetLayerPriority(Entry.Layer);
	Entry.Order = NextMenuOrder++;
	Menu->Owner = this;
	Menu->MenuLayer = Entry.Layer;
	Menu->bRemoving = false;

	if (!Menu->IsInViewport())
	{
		const int32 ZOrder = Entry.Priority * 100 + (Entry.Order % 100);
		if (Menu->GetOwningPlayer())
		{
			Menu->AddToPlayerScreen(ZOrder);
		}
		else
		{
			Menu->AddToViewport(ZOrder);
		}
	}
	if (Menu->Anim_In)
	{
		Menu->PlayAnimation(Menu->Anim_In);
	}
	SetDeviceTrackingEnabled(true);
	RefreshTopMenu();
	return true;
}

bool UCoreKitNavigationSubsystem::PopMenu(UCoreKitMenuWidget* Menu)
{
	const int32 Index = Menus.IndexOfByPredicate([Menu](const FMenuEntry& Entry) { return Entry.Menu.Get() == Menu; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	const FMenuEntry Removed = Menus[Index];
	Menus.RemoveAt(Index);
	if (ActiveMenu.Get() == Menu)
	{
		ActiveMenu.Reset();
		Menu->SetMenuActive(false);
	}
	if (Removed.FocusScope.IsValid())
	{
		ReleaseMenuFocusScope(Removed.FocusScope);
	}
	RefreshTopMenu();
	if (IsValid(Menu))
	{
		Menu->BeginRemoval();
	}
	return true;
}

void UCoreKitNavigationSubsystem::PopAllMenus(FName Layer)
{
	TArray<UCoreKitMenuWidget*> ToPop;
	for (int32 Index = Menus.Num() - 1; Index >= 0; --Index)
	{
		if (Layer.IsNone() || Menus[Index].Layer == Layer)
		{
			if (UCoreKitMenuWidget* Menu = Menus[Index].Menu.Get())
			{
				ToPop.Add(Menu);
			}
		}
	}
	for (UCoreKitMenuWidget* Menu : ToPop)
	{
		PopMenu(Menu);
	}
	Menus.RemoveAll([](const FMenuEntry& Entry) { return !Entry.Menu.IsValid(); });
	RefreshTopMenu();
}

bool UCoreKitNavigationSubsystem::IsMenuOpen(const UCoreKitMenuWidget* Menu) const
{
	return Menu && Menus.ContainsByPredicate([Menu](const FMenuEntry& Entry) { return Entry.Menu.Get() == Menu; });
}

UCoreKitMenuWidget* UCoreKitNavigationSubsystem::GetTopMenu() const
{
	const FMenuEntry* Best = nullptr;
	for (const FMenuEntry& Entry : Menus)
	{
		if (Entry.Menu.IsValid() && (!Best || Entry.Priority > Best->Priority || (Entry.Priority == Best->Priority && Entry.Order > Best->Order)))
		{
			Best = &Entry;
		}
	}
	return Best ? Best->Menu.Get() : nullptr;
}

UCoreKitMenuWidget* UCoreKitNavigationSubsystem::GetTopMenuInLayer(FName Layer) const
{
	const FMenuEntry* Best = nullptr;
	for (const FMenuEntry& Entry : Menus)
	{
		if (Entry.Layer == Layer && Entry.Menu.IsValid() && (!Best || Entry.Order > Best->Order))
		{
			Best = &Entry;
		}
	}
	return Best ? Best->Menu.Get() : nullptr;
}

void UCoreKitNavigationSubsystem::RefreshTopMenu()
{
	Menus.RemoveAll([](const FMenuEntry& Entry) { return !Entry.Menu.IsValid(); });
	UCoreKitMenuWidget* Top = GetTopMenu();
	if (Top == ActiveMenu.Get())
	{
		ApplyMenuInput();
		return;
	}
	if (UCoreKitMenuWidget* Previous = ActiveMenu.Get())
	{
		Previous->SetMenuActive(false);
	}
	ActiveMenu = Top;
	ApplyMenuInput();
	if (Top)
	{
		Top->SetMenuActive(true);
		FMenuEntry* Entry = Menus.FindByPredicate([Top](const FMenuEntry& Item) { return Item.Menu.Get() == Top; });
		if (Entry && !Entry->FocusScope.IsValid())
		{
			Top->ForceLayoutPrepass();
			if (UWidget* Focus = Top->GetInitialFocus())
			{
				Entry->FocusScope = PushMenuFocusScope(Focus, Top);
			}
		}
		else if (Entry)
		{
			// Returning to a menu that already had focus: restore its scope target.
			if (const FMenuFocusScope* Scope = FocusScopes.FindByPredicate([Entry](const FMenuFocusScope& Item) { return Item.Id == Entry->FocusScope; }))
			{
				if (const TSharedPtr<SWidget> Target = Scope->Target.Pin(); Target && FSlateApplication::IsInitialized())
				{
					if (const TSharedPtr<FSlateUser> User = GetLocalPlayer()->GetSlateUser())
					{
						FSlateApplication::Get().SetUserFocus(User->GetUserIndex(), Target);
					}
				}
			}
		}
	}
	OnTopMenuChanged.Broadcast(Top);
}

void UCoreKitNavigationSubsystem::ApplyMenuInput()
{
	APlayerController* Controller = GetMenuPlayerController();
	if (!Controller)
	{
		return;
	}
	const UCoreKitMenuWidget* Top = ActiveMenu.Get();
	if (!Top || Top->InputMode == ECoreKitMenuInputMode::Game)
	{
		if (bMenuInputApplied)
		{
			Controller->SetInputMode(FInputModeGameOnly());
			Controller->SetShowMouseCursor(bSavedShowCursor);
			bMenuInputApplied = false;
		}
		return;
	}
	if (!bMenuInputApplied)
	{
		bSavedShowCursor = Controller->ShouldShowMouseCursor();
		bMenuInputApplied = true;
	}
	if (Top->InputMode == ECoreKitMenuInputMode::Menu)
	{
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(Mode);
	}
	else
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Controller->SetInputMode(Mode);
	}
	const bool bHideForGamepad = GetDefault<UCoreKitNavigationSettings>()->bAutoHideCursorWithGamepad && InputDevice == ECoreKitInputDevice::Gamepad;
	Controller->SetShowMouseCursor(Top->bShowCursor && !bHideForGamepad);
}

bool UCoreKitNavigationSubsystem::HandleMenuKey(uint32 SlateUserIndex, FKey Key, bool bRepeat)
{
	UCoreKitMenuWidget* Top = ActiveMenu.Get();
	if (!Top || bCapturingKey)
	{
		return false;
	}
	const TSharedPtr<FSlateUser> User = GetLocalPlayer()->GetSlateUser();
	if (!User || SlateUserIndex != static_cast<uint32>(User->GetUserIndex()))
	{
		return false;
	}
	const UCoreKitNavigationSettings* Settings = GetDefault<UCoreKitNavigationSettings>();
	const bool bReturn = Settings->ReturnKeys.Contains(Key);
	if (Top->IsInputCoolingDown() && (bReturn || Settings->AcceptKeys.Contains(Key)))
	{
		return true;
	}
	if (bRepeat)
	{
		return false;
	}
	if (bReturn)
	{
		if (Top->HandleReturn())
		{
			if (USoundBase* Sound = Settings->ReturnSound.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(Top, Sound);
			}
			return true;
		}
		return false;
	}
	if (Settings->NextSectionKeys.Contains(Key))
	{
		return Top->HandleSection(1);
	}
	if (Settings->PreviousSectionKeys.Contains(Key))
	{
		return Top->HandleSection(-1);
	}
	return Top->HandleMenuKey(Key);
}

UCoreKitPromptWidget* UCoreKitNavigationSubsystem::PushPrompt(TSubclassOf<UCoreKitPromptWidget> PromptClass, FText Title, FText Body,
	const TArray<FText>& Options, int32 CancelOption, int32 DefaultOption, FCoreKitPromptCallback Callback)
{
	APlayerController* Controller = GetMenuPlayerController();
	UClass* Class = PromptClass ? PromptClass.Get() : UCoreKitPromptWidget::StaticClass();
	UCoreKitPromptWidget* Prompt = Controller
		? CreateWidget<UCoreKitPromptWidget>(Controller, Class)
		: CreateWidget<UCoreKitPromptWidget>(GetLocalPlayer()->GetWorld(), Class);
	if (!Prompt)
	{
		return nullptr;
	}
	Prompt->SetupPrompt(Title, Body, Options, CancelOption, DefaultOption);
	Prompt->Callback = Callback;
	PushMenu(Prompt, TEXT("Modal"));
	return Prompt;
}
