#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"

#include "CoreKitFrontend/CoreKitFrontendMenu.h"
#include "CoreKitFrontend/CoreKitFrontendSettings.h"
#include "CoreKitFrontendLoadingWidget.h"
#include "CoreKitLoading/CoreKitLoadingSubsystem.h"
#include "CoreKitLoading/CoreKitOpenLevelRequest.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitSave/CoreKitSaveCatalogRequest.h"
#include "CoreKitSave/CoreKitSaveRequest.h"
#include "CoreKitSave/CoreKitWorldSave.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/Package.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "CoreKitFrontend"

namespace
{
const FName ArrivalCondition(TEXT("CoreKitFrontend.Arrival"));
constexpr const TCHAR* AudioSection = TEXT("CoreKit.Frontend.Audio");
constexpr const TCHAR* AudioKeys[] = { TEXT("Master"), TEXT("Music"), TEXT("Effects") };

const UCoreKitFrontendSettings& Config()
{
	return *GetDefault<UCoreKitFrontendSettings>();
}

bool IsMenuWorld(const UWorld* World)
{
	return World && UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == Config().MainMenuMap.ToSoftObjectPath().GetLongPackageName();
}

TSoftObjectPtr<UWorld> SavedMap(const UCoreKitWorldSaveGame& Save)
{
	const FString Package = Save.GetSavedWorldPackage().ToString();
	if (!FPackageName::IsValidLongPackageName(Package) || !FPackageName::DoesPackageExist(Package))
	{
		return {};
	}
	return TSoftObjectPtr<UWorld>(FSoftObjectPath(Package + TEXT(".") + FPackageName::GetLongPackageAssetName(Package)));
}

FString SlotNamespace()
{
	return Config().SlotPrefix + TEXT("-");
}
}

TSharedRef<SWidget> UCoreKitFrontendLoadingWidget::RebuildWidget()
{
	return SNew(SBorder).BorderBackgroundColor(FLinearColor(.02f, .03f, .05f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(STextBlock).Text(LOCTEXT("Loading", "Loading…"))];
}

bool UCoreKitFrontendSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && Config().bEnabled;
}

void UCoreKitFrontendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UCoreKitLoadingSubsystem>();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AudioKeys); ++Index)
	{
		float Volume = 1.f;
		GConfig->GetFloat(AudioSection, AudioKeys[Index], Volume, GGameUserSettingsIni);
		AudioVolumes[Index] = FMath::IsFinite(Volume) ? FMath::Clamp(Volume, 0.f, 1.f) : 1.f;
	}
	FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::HandleWorldStarted);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandleMapLoaded);
}

void UCoreKitFrontendSubsystem::Deinitialize()
{
	bShuttingDown = true;
	FWorldDelegates::OnStartGameInstance.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FTSTicker::RemoveTicker(WorldTicker);
	FTSTicker::RemoveTicker(ArrivalTicker);
	RemoveMenu(false);
	if (AudioWorld.IsValid() && ActiveAudioMix.IsValid())
	{
		UGameplayStatics::PopSoundMixModifier(AudioWorld.Get(), ActiveAudioMix.Get());
	}
	Super::Deinitialize();
}

APlayerController* UCoreKitFrontendSubsystem::FindController(UWorld* World) const
{
	auto* Player = GetGameInstance()->GetFirstGamePlayer();
	return Player && World ? Player->GetPlayerController(World) : nullptr;
}

void UCoreKitFrontendSubsystem::HandleWorldStarted(UGameInstance* Game)
{
	if (Game == GetGameInstance())
	{
		QueueWorldPresentation(Game->GetWorld());
	}
}

void UCoreKitFrontendSubsystem::HandleMapLoaded(UWorld* World)
{
	if (World && World->GetGameInstance() == GetGameInstance())
	{
		if (Menu && (!MenuController.IsValid() || MenuController->GetWorld() != World))
		{
			RemoveMenu(false);
			UpdateState(ECoreKitFrontendState::Gameplay);
		}
		QueueWorldPresentation(World);
	}
}

void UCoreKitFrontendSubsystem::QueueWorldPresentation(UWorld* World)
{
	FTSTicker::RemoveTicker(WorldTicker);
	WorldTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
		[this, Target = TWeakObjectPtr(World), Deadline = FPlatformTime::Seconds() + 10.0](float)
		{
			UWorld* Current = Target.Get();
			if (!Current || Current != GetWorld() || FPlatformTime::Seconds() >= Deadline)
			{
				return false;
			}
			if (!FindController(Current) || !Current->HasBegunPlay())
			{
				return true;
			}
			ApplyAudio(Current, AudioVolumes);
			if (State != ECoreKitFrontendState::Traveling && IsMenuWorld(Current) && !Menu)
			{
				PresentMenu(Current);
			}
			return false;
		}));
}

void UCoreKitFrontendSubsystem::PresentMenu(UWorld* World, const FText& Message)
{
	APlayerController* Controller = FindController(World);
	if (!Controller || !GetGameInstance()->GetGameViewportClient())
	{
		UpdateState(ECoreKitFrontendState::Gameplay, Message);
		return;
	}
	RemoveMenu(true);
	Menu = CreateWidget<UCoreKitFrontendMenu>(Controller, Config().MenuWidgetClass
		? Config().MenuWidgetClass.Get() : UCoreKitFrontendMenu::StaticClass());
	if (!Menu || !Menu->AddToPlayerScreen(100))
	{
		Menu = nullptr;
		UpdateState(ECoreKitFrontendState::Gameplay, Message);
		return;
	}
	MenuController = Controller;
	bPreviousMouseCursor = Controller->ShouldShowMouseCursor();
	Controller->SetShowMouseCursor(true);
	FInputModeUIOnly InputMode;
	Controller->SetInputMode(InputMode);
	bHasCheckpoint = false;
	UpdateState(ECoreKitFrontendState::MainMenu, Message);
	RefreshMenuFocus();
	RefreshCheckpointAvailability();
}

void UCoreKitFrontendSubsystem::RemoveMenu(bool bReleaseInput)
{
	if (APlayerController* Controller = MenuController.Get())
	{
		if (auto* Player = Controller->GetLocalPlayer())
		{
			Player->GetSubsystem<UCoreKitNavigationSubsystem>()->ReleaseMenuFocusScope(FocusScope);
		}
		if (bReleaseInput)
		{
			Controller->SetShowMouseCursor(bPreviousMouseCursor);
			Controller->SetInputMode(FInputModeGameOnly());
		}
	}
	FocusScope.Invalidate();
	MenuController.Reset();
	if (Menu)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}
	++CatalogGeneration;
}

void UCoreKitFrontendSubsystem::RefreshMenuFocus()
{
	APlayerController* Controller = MenuController.Get();
	if (Controller && Menu)
	{
		auto* Nav = Controller->GetLocalPlayer()->GetSubsystem<UCoreKitNavigationSubsystem>();
		Nav->ReleaseMenuFocusScope(FocusScope);
		FocusScope = Nav->PushMenuFocusScope(Menu->GetInitialFocus(), Menu);
	}
}

void UCoreKitFrontendSubsystem::UpdateState(ECoreKitFrontendState NextState, const FText& Message)
{
	State = NextState;
	Status = Message;
	OnChanged.Broadcast();
}

void UCoreKitFrontendSubsystem::RefreshCheckpointAvailability()
{
	const uint32 Generation = ++CatalogGeneration;
	auto* Request = UCoreKitSaveCatalogRequest::ListSlots(this);
	Request->CompletedNative.AddWeakLambda(this, [this, Generation](ECoreKitSaveResult, const TArray<FCoreKitSaveCatalogEntry>& Entries)
	{
		if (Generation != CatalogGeneration || !Menu)
		{
			return;
		}
		const FString Prefix = SlotNamespace();
		bHasCheckpoint = Entries.ContainsByPredicate([&Prefix](const FCoreKitSaveCatalogEntry& Entry)
		{
			return Entry.Result == ECoreKitSaveResult::Success && Entry.Info.SlotName.StartsWith(Prefix);
		});
		OnChanged.Broadcast();
	});
	Request->Activate();
}

void UCoreKitFrontendSubsystem::NewGame()
{
	if (State == ECoreKitFrontendState::MainMenu)
	{
		if (Config().FirstGameMap.ToSoftObjectPath().GetLongPackageName() == Config().MainMenuMap.ToSoftObjectPath().GetLongPackageName())
		{
			UpdateState(ECoreKitFrontendState::MainMenu, LOCTEXT("DistinctMaps", "Set a first game map distinct from the main menu."));
			return;
		}
		ActiveCheckpoint = nullptr;
		StartTravel(Config().FirstGameMap, false);
	}
}

void UCoreKitFrontendSubsystem::ContinueGame()
{
	if (!CanContinue())
	{
		return;
	}
	UpdateState(ECoreKitFrontendState::ReadingCheckpoint, LOCTEXT("Reading", "Checking checkpoints…"));
	auto* Request = UCoreKitSaveCatalogRequest::ListSlots(this);
	Request->CompletedNative.AddWeakLambda(this, [this](ECoreKitSaveResult, const TArray<FCoreKitSaveCatalogEntry>& Entries)
	{
		if (State != ECoreKitFrontendState::ReadingCheckpoint || bShuttingDown)
		{
			return;
		}
		CheckpointCandidates.Reset();
		NextCandidate = 0;
		LatestVerified = FDateTime::MinValue();
		const FString Prefix = SlotNamespace();
		for (const FCoreKitSaveCatalogEntry& Entry : Entries)
		{
			if (Entry.Result == ECoreKitSaveResult::Success && Entry.Info.SlotName.StartsWith(Prefix))
			{
				CheckpointCandidates.Emplace(Entry.Info.SlotName, Entry.Info.WrittenAtUtc);
			}
		}
		ReadNextCheckpoint();
	});
	Request->Activate();
}

void UCoreKitFrontendSubsystem::ReadNextCheckpoint()
{
	if (!CheckpointCandidates.IsValidIndex(NextCandidate)
		|| (PendingSnapshot && CheckpointCandidates[NextCandidate].Value <= LatestVerified))
	{
		CheckpointCandidates.Reset();
		if (PendingSnapshot)
		{
			StartTravel(SavedMap(*PendingSnapshot), false);
		}
		else
		{
			UpdateState(ECoreKitFrontendState::MainMenu, LOCTEXT("NoReadableSave", "No checkpoint could be loaded."));
			RefreshCheckpointAvailability();
		}
		return;
	}

	auto* Request = UCoreKitSaveRequest::LoadCoreKitSlot(this, CheckpointCandidates[NextCandidate++].Key);
	Request->CompletedNative.AddWeakLambda(this, [this](ECoreKitSaveResult Result, USaveGame* Save, const FCoreKitSaveSlotInfo& Info)
	{
		if (State != ECoreKitFrontendState::ReadingCheckpoint || bShuttingDown)
		{
			return;
		}
		auto* Snapshot = Cast<UCoreKitWorldSaveGame>(Save);
		if (Result == ECoreKitSaveResult::Success && Snapshot && !SavedMap(*Snapshot).IsNull() && Info.WrittenAtUtc > LatestVerified)
		{
			PendingSnapshot = Snapshot;
			LatestVerified = Info.WrittenAtUtc;
			bRecoveredCheckpoint = Info.bRecoveredBackup;
		}
		ReadNextCheckpoint();
	});
	Request->Activate();
}

void UCoreKitFrontendSubsystem::OpenSettings()
{
	if (State == ECoreKitFrontendState::MainMenu)
	{
		UpdateState(ECoreKitFrontendState::Settings);
		RefreshMenuFocus();
	}
}

void UCoreKitFrontendSubsystem::CloseSettings()
{
	if (State == ECoreKitFrontendState::Settings)
	{
		UpdateState(ECoreKitFrontendState::MainMenu);
		RefreshMenuFocus();
	}
}

void UCoreKitFrontendSubsystem::ReturnToMainMenu()
{
	if (State == ECoreKitFrontendState::Gameplay && !bSaving)
	{
		StartTravel(Config().MainMenuMap, true);
	}
}

bool UCoreKitFrontendSubsystem::RestartFromCheckpoint()
{
	if (!CanRestartFromCheckpoint() || SavedMap(*ActiveCheckpoint).IsNull())
	{
		return false;
	}
	PendingSnapshot = ActiveCheckpoint;
	StartTravel(SavedMap(*PendingSnapshot), false);
	return true;
}

bool UCoreKitFrontendSubsystem::SaveCheckpoint(const FString& Slot, bool bQueueIfBusy)
{
	if (State != ECoreKitFrontendState::Gameplay || Slot.IsEmpty() || !GetWorld()
		|| (bSaving && (!bQueueIfBusy || (QueuedCheckpoint && QueuedSlot != Slot))))
	{
		return false;
	}
	auto* WorldSave = GetWorld()->GetSubsystem<UCoreKitWorldSaveSubsystem>();
	auto* Snapshot = NewObject<UCoreKitWorldSaveGame>(this);
	if (!WorldSave || WorldSave->CaptureWorld(Snapshot).Result != ECoreKitWorldSaveResult::Success)
	{
		return false;
	}
	ActiveCheckpoint = Snapshot;
	if (bSaving)
	{
		QueuedCheckpoint = Snapshot;
		QueuedSlot = Slot;
	}
	else
	{
		WriteCheckpoint(Slot, Snapshot);
	}
	return true;
}

void UCoreKitFrontendSubsystem::WriteCheckpoint(const FString& Slot, UCoreKitWorldSaveGame* Snapshot)
{
	bSaving = true;
	auto* Request = UCoreKitSaveRequest::SaveCoreKitSlot(this, SlotNamespace() + Slot, Snapshot);
	Request->CompletedNative.AddWeakLambda(this, [this](ECoreKitSaveResult Result, USaveGame*, const FCoreKitSaveSlotInfo&)
	{
		bSaving = false;
		if (QueuedCheckpoint)
		{
			UCoreKitWorldSaveGame* Next = QueuedCheckpoint;
			QueuedCheckpoint = nullptr;
			WriteCheckpoint(QueuedSlot, Next);
		}
		OnCheckpointSaved.Broadcast(Result == ECoreKitSaveResult::Success);
	});
	Request->Activate();
}

void UCoreKitFrontendSubsystem::StartTravel(TSoftObjectPtr<UWorld> Destination, bool bToMenu, const FText& ReturnMessage)
{
	if (bShuttingDown)
	{
		return;
	}
	const bool bContinuing = PendingSnapshot != nullptr;
	bProjectReadyRequested = false;
	bSkippedParticipants = false;
	RemoveMenu(true);
	UpdateState(ECoreKitFrontendState::Traveling);
	auto* Loading = GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>();
	Loading->ConfigureLoadingScreen(Config().LoadingWidgetClass ? Config().LoadingWidgetClass.Get()
		: UCoreKitFrontendLoadingWidget::StaticClass(), false);
	const TArray<FName> Conditions = bToMenu ? TArray<FName>{} : TArray<FName>{ArrivalCondition};
	auto* Request = UCoreKitOpenLevelRequest::OpenLevelWithLoading(this, Destination, Conditions, Config().TravelTimeoutSeconds);
	Request->MapLoadedNative.AddWeakLambda(this, [this, bToMenu](ECoreKitTravelResult, FGuid Id)
	{
		if (!bToMenu)
		{
			PendingTravelId = Id;
			WaitForArrival(Id);
		}
	});
	Request->CompletedNative.AddWeakLambda(this, [this, bToMenu, bContinuing, ReturnMessage](ECoreKitTravelResult Result, FGuid)
	{
		FTSTicker::RemoveTicker(ArrivalTicker);
		PendingTravelId.Invalidate();
		if (Result == ECoreKitTravelResult::Success && bContinuing)
		{
			ActiveCheckpoint = PendingSnapshot;
		}
		PendingSnapshot = nullptr;
		const bool bRecovered = bRecoveredCheckpoint;
		bRecoveredCheckpoint = false;
		if (bShuttingDown)
		{
			return;
		}
		UWorld* World = GetWorld();
		if (Result != ECoreKitTravelResult::Success)
		{
			const FText Error = LOCTEXT("TravelFailed", "The game could not enter the requested map or restore the checkpoint.");
			if (!bToMenu && World && !IsMenuWorld(World))
			{
				StartTravel(Config().MainMenuMap, true, Error);
			}
			else
			{
				PresentMenu(World, ReturnMessage.IsEmpty() ? Error : ReturnMessage);
			}
		}
		else if (bToMenu)
		{
			PresentMenu(World, ReturnMessage);
		}
		else
		{
			FText Message;
			if (bRecovered && bSkippedParticipants)
			{
				Message = LOCTEXT("BackupAndMissing", "Recovered a backup checkpoint. Some saved actors are missing.");
			}
			else if (bRecovered)
			{
				Message = LOCTEXT("Recovered", "Recovered a backup checkpoint.");
			}
			else if (bSkippedParticipants)
			{
				Message = LOCTEXT("MissingActors", "Some saved actors are missing.");
			}
			UpdateState(ECoreKitFrontendState::Gameplay, Message);
			OnGameArrived.Broadcast(bContinuing);
		}
	});
	Request->Activate();
}

void UCoreKitFrontendSubsystem::WaitForArrival(FGuid TravelId)
{
	ArrivalTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, TravelId](float)
	{
		APlayerController* Controller = FindController(GetWorld());
		if (!Controller || !Controller->GetWorld()->HasBegunPlay() || (Config().bRequirePossessedPawn && !Controller->GetPawn()))
		{
			return true;
		}
		ArrivalTicker.Reset();
		if (Config().bWaitForProjectReady)
		{
			bProjectReadyRequested = true;
			OnPrepareArrival.Broadcast(TravelId, PendingSnapshot != nullptr);
		}
		else
		{
			RestoreArrival(TravelId);
		}
		return false;
	}));
}

bool UCoreKitFrontendSubsystem::ResolveProjectReady(FGuid TravelId, bool bSucceeded)
{
	if (!bProjectReadyRequested || !TravelId.IsValid() || TravelId != PendingTravelId)
	{
		return false;
	}
	bProjectReadyRequested = false;
	if (bSucceeded)
	{
		RestoreArrival(TravelId);
	}
	else
	{
		GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>()->ResolveTravelCondition(TravelId, ArrivalCondition, false);
	}
	return true;
}

void UCoreKitFrontendSubsystem::RestoreArrival(FGuid TravelId)
{
	bool bSucceeded = true;
	if (PendingSnapshot)
	{
		auto* WorldSave = GetWorld() ? GetWorld()->GetSubsystem<UCoreKitWorldSaveSubsystem>() : nullptr;
		const auto Result = WorldSave ? WorldSave->RestoreWorld(PendingSnapshot, Config().bAllowMissingParticipants).Result
			: ECoreKitWorldSaveResult::InvalidSnapshot;
		bSkippedParticipants = Result == ECoreKitWorldSaveResult::SuccessWithMissing;
		bSucceeded = Result == ECoreKitWorldSaveResult::Success || bSkippedParticipants;
	}
	GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>()->ResolveTravelCondition(TravelId, ArrivalCondition, bSucceeded);
}

void UCoreKitFrontendSubsystem::ApplyAudio(UWorld* World, const FVector3f& Volumes)
{
	USoundMix* Mix = Config().UserVolumeMix.LoadSynchronous();
	if (!World || !Mix)
	{
		return;
	}
	USoundClass* Classes[] = { Config().MasterSoundClass.LoadSynchronous(),
		Config().MusicSoundClass.LoadSynchronous(), Config().EffectsSoundClass.LoadSynchronous() };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Classes); ++Index)
	{
		if (Classes[Index])
		{
			UGameplayStatics::SetSoundMixClassOverride(World, Mix, Classes[Index], Volumes[Index], 1.f, .15f, true);
		}
	}
	if (AudioWorld.Get() != World || ActiveAudioMix.Get() != Mix)
	{
		if (AudioWorld.IsValid() && ActiveAudioMix.IsValid())
		{
			UGameplayStatics::PopSoundMixModifier(AudioWorld.Get(), ActiveAudioMix.Get());
		}
		UGameplayStatics::PushSoundMixModifier(World, Mix);
		AudioWorld = World;
		ActiveAudioMix = Mix;
	}
}

void UCoreKitFrontendSubsystem::SaveAudio(const FVector3f& Volumes)
{
	AudioVolumes = Volumes;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(AudioKeys); ++Index)
	{
		GConfig->SetFloat(AudioSection, AudioKeys[Index], Volumes[Index], GGameUserSettingsIni);
	}
	GConfig->Flush(false, GGameUserSettingsIni);
	ApplyAudio(GetWorld(), Volumes);
}

#undef LOCTEXT_NAMESPACE
