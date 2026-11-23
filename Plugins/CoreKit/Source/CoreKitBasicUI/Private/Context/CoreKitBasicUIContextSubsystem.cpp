#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"

#include "Engine/GameInstance.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "CoreKitBasicUI/Events/CoreKitBasicUIEvents.h"
#include "GameFramework/PlayerController.h"
#include "MVVMGameSubsystem.h"
#include "CoreKitBasicUI/Presenters/CoreKitBasicUIPresenter.h"
#include "Camera/PlayerCameraManager.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitBasicUI/Effects/CoreKitBasicUIScreenEffectsComponent.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUIUserSettings.h"
#include "Types/MVVMViewModelCollection.h"

bool UCoreKitBasicUIContextSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return GetDefault<UCoreKitBasicUISettings>()->bEnabled && Super::ShouldCreateSubsystem(Outer);
}

void UCoreKitBasicUIContextSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UCoreKitNavigationSubsystem>();
	Collection.InitializeDependency<UCoreKitBasicUIEventChannel>();
	CreateViewModels();
	RegisterViewModels();
	// Settings apply on game start (engine-wide values and the preferences); world values follow at run start.
	UCoreKitBasicUIUserSettings* Settings = UCoreKitBasicUIUserSettings::Get();
	Settings->ApplyStartup(GetLocalPlayer());
	// This context may not be reachable through the local player yet while it initializes: write its preferences here.
	Preferences->SetHudScale(Settings->GetHudScaleFactor());
	Preferences->SetCrosshairColor(Settings->GetCrosshairLinearColor());
	Preferences->SetbReducedMotion(Settings->bReducedMotion);
	Preferences->SetbLowHealthHeartbeat(Settings->bLowHealthHeartbeat);
	Preferences->SetbDamageNumbers(Settings->bDamageNumbers);
	Preferences->SetScreenShake(Settings->GetScreenShakeScale());
	Loadout->SetCrosshairColor(Settings->GetCrosshairLinearColor());
}

void UCoreKitBasicUIContextSubsystem::CreateViewModels()
{
	Run = NewObject<UCoreKitBasicUIRunViewModel>(this);
	const float SessionSeconds = FMath::Max(1.f, GetDefault<UCoreKitBasicUISettings>()->DefaultSessionSeconds);
	Run->SetSurvivalSeconds(SessionSeconds);
	Run->SetRemainingSeconds(SessionSeconds);
	Run->SetStageStarts({0.f, SessionSeconds * .2f, SessionSeconds * .5f, SessionSeconds * .8f, SessionSeconds});
	Vitals = NewObject<UCoreKitBasicUIVitalsViewModel>(this);
	Loadout = NewObject<UCoreKitBasicUILoadoutViewModel>(this);
	Progression = NewObject<UCoreKitBasicUIProgressionViewModel>(this);
	Build = NewObject<UCoreKitBasicUIBuildViewModel>(this);
	Boss = NewObject<UCoreKitBasicUIBossViewModel>(this);
	Minimap = NewObject<UCoreKitBasicUIMinimapViewModel>(this);
	Summary = NewObject<UCoreKitBasicUIRunSummaryViewModel>(this);
	Preferences = NewObject<UCoreKitBasicUIPreferencesViewModel>(this);
    Run->TogglePause.BindWeakLambda(this, [this]{ OnCommandRequested.Broadcast(TEXT("TogglePause")); });
    Run->Restart.BindWeakLambda(this, [this]{ OnCommandRequested.Broadcast(TEXT("Restart")); });
    Run->QuitToMenu.BindWeakLambda(this, [this]{ OnCommandRequested.Broadcast(TEXT("QuitToMenu")); });
    Progression->ChooseCard.BindWeakLambda(this, [this](int32 Index) { return CardChoiceHandler.IsBound() && CardChoiceHandler.Execute(Index); });
}

UCoreKitBasicUIContextSubsystem* UCoreKitBasicUIContextSubsystem::CreateDetachedForTests(UObject* Outer)
{
	// The subsystem is Within=LocalPlayer: give detached test contexts a transient local player as outer.
	if (!Cast<ULocalPlayer>(Outer))
	{
		Outer = NewObject<ULocalPlayer>(GEngine, NAME_None, RF_Transient);
	}
	UCoreKitBasicUIContextSubsystem* Context = NewObject<UCoreKitBasicUIContextSubsystem>(Outer);
	Context->bDetached = true;
	Context->CreateViewModels();
	return Context;
}

void UCoreKitBasicUIContextSubsystem::Deinitialize()
{
	StopRunPresentation();
	UnregisterViewModels();
	Super::Deinitialize();
}

UCoreKitBasicUIEventChannel* UCoreKitBasicUIContextSubsystem::GetEvents() const
{
	return !bDetached && GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UCoreKitBasicUIEventChannel>() : nullptr;
}

void UCoreKitBasicUIContextSubsystem::RegisterViewModels()
{
	const UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	UMVVMGameSubsystem* Mvvm = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	if (!GetDefault<UCoreKitBasicUISettings>()->bRegisterGlobalViewModels || !Mvvm || bRegistered)
	{
		return;
	}
	UMVVMViewModelCollectionObject* Collection = Mvvm->GetViewModelCollection();
	auto Add = [this, Collection](UMVVMViewModelBase* ViewModel, const TCHAR* Name)
	{
		FMVVMViewModelContext Context;
		Context.ContextClass = ViewModel->GetClass();
		Context.ContextName = FName(*FString::Printf(TEXT("%s.Player%d"), Name, GetLocalPlayer()->GetLocalPlayerIndex()));
		Collection->AddViewModelInstance(Context, ViewModel);
	};
	Add(Run, TEXT("CoreKitBasicUI.Run"));
	Add(Vitals, TEXT("CoreKitBasicUI.Vitals"));
	Add(Loadout, TEXT("CoreKitBasicUI.Loadout"));
	Add(Progression, TEXT("CoreKitBasicUI.Progression"));
	Add(Build, TEXT("CoreKitBasicUI.Build"));
	Add(Boss, TEXT("CoreKitBasicUI.Boss"));
	Add(Minimap, TEXT("CoreKitBasicUI.Minimap"));
	Add(Summary, TEXT("CoreKitBasicUI.Summary"));
	Add(Preferences, TEXT("CoreKitBasicUI.Preferences"));
	bRegistered = true;
}

void UCoreKitBasicUIContextSubsystem::UnregisterViewModels()
{
	const UGameInstance* GameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
	UMVVMGameSubsystem* Mvvm = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	if (!Mvvm || !bRegistered)
	{
		return;
	}
	UMVVMViewModelCollectionObject* Collection = Mvvm->GetViewModelCollection();
	for (UMVVMViewModelBase* ViewModel : TArray<UMVVMViewModelBase*>{ Run, Vitals, Loadout, Progression, Build, Boss, Minimap, Summary, Preferences })
	{
		Collection->RemoveAllViewModelInstance(ViewModel);
	}
	bRegistered = false;
}

void UCoreKitBasicUIContextSubsystem::StartRunPresentation(APlayerController* Controller)
{
	StopRunPresentation();
	if (!Controller)
	{
		return;
	}
	// World-side effects live on the run's camera manager (not for detached test contexts: no engine side effects).
	if (APlayerCameraManager* Camera = Controller->PlayerCameraManager; Camera && GetDefault<UCoreKitBasicUISettings>()->bEnableCameraEffects && !bDetached && !Camera->FindComponentByClass<UCoreKitBasicUIScreenEffectsComponent>())
	{
		UCoreKitBasicUIScreenEffectsComponent* Effects = NewObject<UCoreKitBasicUIScreenEffectsComponent>(Camera, TEXT("CoreKitBasicUIScreenEffects"));
		Camera->AddInstanceComponent(Effects);
		Effects->RegisterComponent();
	}
	if (!bDetached)
	{
		UCoreKitBasicUIUserSettings::Get()->ApplyNonVideo(Controller);
	}

	for (TSubclassOf<UCoreKitBasicUIPresenter> Class : GetDefault<UCoreKitBasicUISettings>()->PresenterClasses)
	{
		if (Class && !Class->HasAnyClassFlags(CLASS_Abstract)) Presenters.Add(NewObject<UCoreKitBasicUIPresenter>(this, Class));
	}
	for (UCoreKitBasicUIPresenter* Presenter : Presenters)
	{
		Presenter->Start(this, Controller);
	}
	// About 30 Hz on the core ticker: real time, also while the world is paused (cards, pause, end screens).
	if (!bDetached)
	{
		PresenterTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickFromTicker), 1.f / 30.f);
	}
	TickPresenters(0.f);
}

bool UCoreKitBasicUIContextSubsystem::TickFromTicker(float DeltaSeconds)
{
	TickPresenters(DeltaSeconds);
	return true;
}

void UCoreKitBasicUIContextSubsystem::TickPresenters(float DeltaSeconds)
{
	for (UCoreKitBasicUIPresenter* Presenter : Presenters)
	{
		if (Presenter)
		{
			Presenter->Tick(DeltaSeconds);
		}
	}
}

void UCoreKitBasicUIContextSubsystem::StopRunPresentation()
{
	if (PresenterTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PresenterTicker);
		PresenterTicker.Reset();
	}
	for (UCoreKitBasicUIPresenter* Presenter : Presenters)
	{
		if (Presenter)
		{
			Presenter->Stop();
		}
	}
	Presenters.Reset();
}
