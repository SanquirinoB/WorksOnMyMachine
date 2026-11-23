#include "CoreKitBasicUI/FrontEnd/CoreKitBasicUIMenuGameMode.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/CanvasPanel.h"
#include "CoreKitFrontend/CoreKitFrontendSettings.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "CoreKitLoading/CoreKitLoadingSubsystem.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "EngineUtils.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "Sound/SoundBase.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUIUserSettings.h"
#include "Sound/SoundClass.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Widgets/FrontEnd/CoreKitBasicUIFrontEndHost.h"
#include "CoreKitBasicUI/Widgets/FrontEnd/CoreKitBasicUIFrontEndMenus.h"



// ---------------------------------------------------------------------------------------------------------------
// Game mode

ACoreKitBasicUIMenuGameMode::ACoreKitBasicUIMenuGameMode()
{
	PlayerControllerClass = ACoreKitBasicUIMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = AHUD::StaticClass();
	bStartPlayersAsSpectators = false;
}

void ACoreKitBasicUIMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// No pawn on the title screen (PostLogin already set the HUD): the controller views the menu camera.
}

// ---------------------------------------------------------------------------------------------------------------
// Controller

ACoreKitBasicUIMenuPlayerController::ACoreKitBasicUIMenuPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShowMouseCursor = true;
}

void ACoreKitBasicUIMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	ACameraActor* Found = nullptr;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("MenuCamera")))
		{
			Found = *It;
			break;
		}
		if (!Found)
		{
			Found = *It;
		}
	}
	if (Found)
	{
		MenuCamera = Found;
		CameraBase = Found->GetActorTransform();
		SetViewTarget(Found);
	}
	DriftStart = FPlatformTime::Seconds();
	RegisterBindings();
	StartMusic();

	// Normally CoreKitFrontend presents the title screen (UCoreKitBasicUIFrontEndHost). Without it (frontend disabled), show the
	// menu directly so the map stays usable; PLAY then opens the arena with a plain OpenLevel.
	const UGameInstance* Game = GetGameInstance();
	if (Game && !Game->GetSubsystem<UCoreKitFrontendSubsystem>())
	{
		ULocalPlayer* Local = GetLocalPlayer();
		if (UCoreKitNavigationSubsystem* Nav = Local ? Local->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr)
		{
			Nav->PushMenu(CreateWidget<UCoreKitBasicUIMainMenu>(this, UCoreKitBasicUIMainMenu::StaticClass()), TEXT("Menu"));
		}
	}
}

void ACoreKitBasicUIMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	// The standalone menu has no Frontend host to remove its stack during travel.
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* Navigation = LocalPlayer->GetSubsystem<UCoreKitNavigationSubsystem>())
		{
			for (FName Layer : { FName(TEXT("Modal")), FName(TEXT("Menu")) })
			{
				while (UCoreKitMenuWidget* Menu = Navigation->GetTopMenuInLayer(Layer))
				{
					if (Menu->GetOwningPlayer() != this || !Navigation->PopMenu(Menu)) break;
				}
			}
		}
	}
	if (Music)
	{
		Music->Stop();
		Music = nullptr;
	}
	Super::EndPlay(Reason);
}

void ACoreKitBasicUIMenuPlayerController::RegisterBindings()
{
	// Keycaps (UCoreKitInputPrompt) read the keys mapped on the player, and rebinding needs the context registered
	// with the Enhanced Input user settings. Without a pawn nothing consumes these actions here.
	Mapping = GetDefault<UCoreKitBasicUISettings>()->InputMapping.LoadSynchronous();
	ULocalPlayer* Local = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Input = Local ? Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Mapping || !Input)
	{
		return;
	}
	if (UEnhancedInputUserSettings* Settings = Input->GetUserSettings())
	{
		Settings->RegisterInputMappingContext(Mapping);
	}
	if (!Input->HasMappingContext(Mapping))
	{
		FModifyContextOptions Options;
		Options.bNotifyUserSettings = true;
		Input->AddMappingContext(Mapping, 0, Options);
	}
}

void ACoreKitBasicUIMenuPlayerController::StartMusic()
{
	USoundBase* Track = GetDefault<UCoreKitBasicUISettings>()->MenuMusic.LoadSynchronous();
	if (!Track || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	Music = UGameplayStatics::CreateSound2D(this, Track, 1.f, 1.f, 0.f, nullptr, false, false);
	if (!Music)
	{
		return;
	}
	// Own sound class: the default class carries the Effects volume (UCoreKitBasicUIUserSettings::ApplyAudio); the menu music
	// follows Master x Music instead, through the volume multiplier (updated every tick, so Settings preview live).
	MusicClass = NewObject<USoundClass>(this, TEXT("CoreKitBasicUIMenuMusicClass"), RF_Transient);
	Music->SoundClassOverride = MusicClass;
	Music->SetLowPassFilterEnabled(true);
	Music->SetLowPassFilterFrequency(LowPassFrequency);
	Music->bIsUISound = true;
	Music->SetVolumeMultiplier(UserMusicVolume());
	Music->FadeIn(MusicFadeIn, MusicVolume, 0.f, EAudioFaderCurve::Linear);
}

float ACoreKitBasicUIMenuPlayerController::UserMusicVolume()
{
	const UCoreKitBasicUIUserSettings* Settings = UCoreKitBasicUIUserSettings::Get();
	return FMath::Clamp(Settings->MasterVolume / 100.f, 0.f, 1.f) * FMath::Clamp(Settings->MusicVolume / 100.f, 0.f, 1.f);
}

void ACoreKitBasicUIMenuPlayerController::FadeOutMusic(float Seconds)
{
	if (Music && Music->IsPlaying())
	{
		Music->FadeOut(FMath::Max(Seconds, 0.01f), 0.f, EAudioFaderCurve::Linear);
	}
}

void ACoreKitBasicUIMenuPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Music)
	{
		Music->SetVolumeMultiplier(UserMusicVolume());
	}
	ACameraActor* Camera = MenuCamera.Get();
	if (!Camera || CoreKitBasicUIMotion::IsReducedMotion())
	{
		return;
	}
	// Slow lateral sway with a slight yaw: a cinematic idle shot of the arena.
	const double T = (FPlatformTime::Seconds() - DriftStart) / FMath::Max(DriftPeriod, 1.f);
	const float Phase = static_cast<float>(T * 2.0 * UE_DOUBLE_PI);
	const FVector Right = CameraBase.GetRotation().GetRightVector();
	const FVector Location = CameraBase.GetLocation() + Right * (FMath::Sin(Phase) * DriftDistance) + FVector(0.f, 0.f, FMath::Sin(Phase * 0.5f) * DriftDistance * 0.15f);
	FRotator Rotation = CameraBase.Rotator();
	Rotation.Yaw += FMath::Sin(Phase + 0.6f) * DriftYaw;
	Camera->SetActorLocationAndRotation(Location, Rotation);
}

// ---------------------------------------------------------------------------------------------------------------
// Frontend host

TSharedRef<SWidget> UCoreKitBasicUIFrontEndHost::RebuildWidget()
{
	// A root of our own keeps CoreKitFrontend from building its built-in title screen.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void UCoreKitBasicUIFrontEndHost::NativeConstruct()
{
	Super::NativeConstruct();
	APlayerController* Controller = GetOwningPlayer();
	ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	if (!MainMenu && Controller && Nav)
	{
		MainMenu = CreateWidget<UCoreKitBasicUIMainMenu>(Controller, UCoreKitBasicUIMainMenu::StaticClass());
		Nav->PushMenu(MainMenu, TEXT("Menu"));
	}
}

void UCoreKitBasicUIFrontEndHost::NativeDestruct()
{
	// Travel removes the host: take the title screen and any screen opened over it with it.
	APlayerController* Controller = GetOwningPlayer();
	ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	if (UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr)
	{
		if (MainMenu)
		{
			Nav->PopAllMenus(TEXT("Modal"));
			Nav->PopAllMenus(TEXT("Menu"));
		}
	}
	MainMenu = nullptr;
	Super::NativeDestruct();
}

UWidget* UCoreKitBasicUIFrontEndHost::GetInitialFocus_Implementation() const
{
	return MainMenu ? MainMenu->GetInitialFocus() : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------
// Flow subsystem

bool UCoreKitBasicUIFrontEndFlowSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && GetDefault<UCoreKitBasicUISettings>()->bEnabled && GetDefault<UCoreKitFrontendSettings>()->bEnabled;
}

void UCoreKitBasicUIFrontEndFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UCoreKitLoadingSubsystem* Loading = Collection.InitializeDependency<UCoreKitLoadingSubsystem>())
	{
		Loading->OnLoadingScreenVisibilityChanged.AddUniqueDynamic(this, &ThisClass::HandleLoadingScreenVisibility);
	}
	LoadingMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandleLoadingMapLoaded);
}

void UCoreKitBasicUIFrontEndFlowSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(LoadingMapHandle);
	FTSTicker::RemoveTicker(LoadingHoldTicker);
	if (UCoreKitLoadingSubsystem* Loading = GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>())
	{
		Loading->OnLoadingScreenVisibilityChanged.RemoveDynamic(this, &ThisClass::HandleLoadingScreenVisibility);
		if (LoadingHoldRequest.IsValid()) Loading->EndLoading(LoadingHoldRequest);
	}
	LoadingHoldRequest.Invalidate();
	Super::Deinitialize();
}

void UCoreKitBasicUIFrontEndFlowSubsystem::HandleLoadingMapLoaded(UWorld* World)
{
	if (World && World->GetGameInstance() == GetGameInstance() && LoadingHoldRequest.IsValid())
	{
		bWaitingForLoadingMap = false;
		LoadingMapReadyAt = FPlatformTime::Seconds();
	}
}

void UCoreKitBasicUIFrontEndFlowSubsystem::HandleLoadingScreenVisibility(bool bVisible)
{
	if (bVisible)
	{
		if (!LoadingHoldRequest.IsValid())
		{
			auto* Loading = GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>();
			bWaitingForLoadingMap = true;
			LoadingHoldRequest = Loading->BeginLoadingRequest(this, FText::GetEmpty());
			LoadingHoldTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
			{
				auto* Current = GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>();
				if (!Current || !LoadingHoldRequest.IsValid()) return false;
				// Preserve the existing screen for 0.2 real seconds after map loading.
				// A failed/cancelled request with no loaded map releases our token as well.
				if (bWaitingForLoadingMap && Current->GetActiveTravelId().IsValid()) return true;
				if (!bWaitingForLoadingMap && FPlatformTime::Seconds() - LoadingMapReadyAt < 0.2) return true;
				LastLoadingHoldSeconds = bWaitingForLoadingMap ? 0.f : static_cast<float>(FPlatformTime::Seconds() - LoadingMapReadyAt);
				const FGuid Request = LoadingHoldRequest;
				LoadingHoldRequest.Invalidate();
				LoadingHoldTicker.Reset();
				Current->EndLoading(Request);
				return false;
			}));
		}
		return;
	}
	UGameInstance* Game = GetGameInstance();
	if (APlayerController* Controller = Game ? Game->GetFirstLocalPlayerController(Game->GetWorld()) : nullptr)
	{
		// Loading restores the viewport state captured before travel. From a defeat
		// screen that state is UI-only, so reapply the destination's game input only
		// after the loading overlay has released it (including the post-load hold).
		ULocalPlayer* Player = Controller->GetLocalPlayer();
		UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
		if (!Controller->IsA<ACoreKitBasicUIMenuPlayerController>() && (!Nav || !Nav->GetTopMenu()))
		{
			Controller->SetInputMode(FInputModeGameOnly());
			Controller->SetShowMouseCursor(false);
		}
		UCoreKitBasicUIScreenWipe::Play(Controller, ECoreKitBasicUIWipeMode::Reveal);
		++RevealCount;
	}
}
