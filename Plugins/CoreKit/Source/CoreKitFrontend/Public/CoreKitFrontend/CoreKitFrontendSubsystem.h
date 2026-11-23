#pragma once

#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoreKitFrontendSubsystem.generated.h"

class APlayerController;
class UCoreKitFrontendMenu;
class UCoreKitWorldSaveGame;
class USoundMix;
class UWorld;

UENUM(BlueprintType)
enum class ECoreKitFrontendState : uint8
{
	Gameplay,
	MainMenu,
	Settings,
	ReadingCheckpoint,
	Traveling
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoreKitFrontendChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitFrontendPrepare, FGuid, TravelId, bool, bContinuing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitFrontendArrived, bool, bContinuing);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitFrontendSaved, bool, bSucceeded);

UCLASS()
class COREKITFRONTEND_API UCoreKitFrontendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool ShouldCreateSubsystem(UObject* Outer) const override;
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "CoreKit|Frontend")
	ECoreKitFrontendState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Frontend")
	FText GetStatus() const { return Status; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Frontend")
	bool CanContinue() const { return State == ECoreKitFrontendState::MainMenu && bHasCheckpoint; }

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	void NewGame();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	void ContinueGame();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	void CloseSettings();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	void ReturnToMainMenu();

	/** Captures the current map and opted-in participants. Completion is reported by OnCheckpointSaved. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	bool SaveCheckpoint(const FString& Slot = TEXT("checkpoint"));

	/** Only needed when Wait For Project Ready is enabled. The ID is valid for this travel only. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Frontend")
	bool ResolveProjectReady(FGuid TravelId, bool bSucceeded);

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Frontend")
	FCoreKitFrontendChanged OnChanged;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Frontend")
	FCoreKitFrontendPrepare OnPrepareArrival;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Frontend")
	FCoreKitFrontendArrived OnGameArrived;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Frontend")
	FCoreKitFrontendSaved OnCheckpointSaved;

private:
	friend class UCoreKitFrontendMenu;

	void HandleWorldStarted(UGameInstance* Game);
	void HandleMapLoaded(UWorld* World);
	void QueueWorldPresentation(UWorld* World);
	void PresentMenu(UWorld* World, const FText& Message = {});
	void RemoveMenu(bool bReleaseInput);
	void RefreshMenuFocus();
	void UpdateState(ECoreKitFrontendState NextState, const FText& Message = {});
	void RefreshCheckpointAvailability();
	void ReadNextCheckpoint();
	void StartTravel(TSoftObjectPtr<UWorld> Destination, bool bToMenu, const FText& ReturnMessage = {});
	void WaitForArrival(FGuid TravelId);
	void RestoreArrival(FGuid TravelId);
	void ApplyAudio(UWorld* World, const FVector3f& Volumes);
	void SaveAudio(const FVector3f& Volumes);
	APlayerController* FindController(UWorld* World) const;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitFrontendMenu> Menu;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitWorldSaveGame> PendingSnapshot;

	TWeakObjectPtr<UWorld> AudioWorld;
	TWeakObjectPtr<USoundMix> ActiveAudioMix;
	TWeakObjectPtr<APlayerController> MenuController;
	FTSTicker::FDelegateHandle WorldTicker;
	FTSTicker::FDelegateHandle ArrivalTicker;
	FGuid FocusScope;
	FGuid PendingTravelId;
	FDateTime LatestVerified;
	FText Status;
	TArray<TPair<FString, FDateTime>> CheckpointCandidates;
	FVector3f AudioVolumes = FVector3f(1.f);
	ECoreKitFrontendState State = ECoreKitFrontendState::Gameplay;
	uint32 CatalogGeneration = 0;
	int32 NextCandidate = 0;
	bool bHasCheckpoint = false;
	bool bSaving = false;
	bool bProjectReadyRequested = false;
	bool bRecoveredCheckpoint = false;
	bool bSkippedParticipants = false;
	bool bPreviousMouseCursor = false;
	bool bShuttingDown = false;
};
