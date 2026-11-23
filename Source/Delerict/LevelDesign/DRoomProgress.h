#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "DRoomProgress.generated.h"

class APawn;
class APlayerController;

/** Progress for the current room and play session. */
UCLASS()
class DELERICT_API UDRoomProgress : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    void Deinitialize() override;
    bool SetCheckpoint(APlayerController& Player, const FTransform& Transform);
    TOptional<FTransform> GetCheckpoint(const APlayerController& Player) const;
    void RestoreCheckpoint(APlayerController& Player, const FTransform& Transform);
    void RequestRespawn(APlayerController& Player, APawn& Pawn);
    bool Retry(APlayerController& Player);
    bool CompleteRoom(APlayerController& Player);
    bool HasCompletedRoom(const APlayerController& Player) const;

private:
    void SaveCheckpoint();
    UFUNCTION() void OnGameArrived(bool bContinuing);
    UFUNCTION() void OnCheckpointSaved(bool bSucceeded);
    bool Respawn(APlayerController& Player);
    bool bSaveFailed = false;

    struct FDPlayerProgress
    {
        TOptional<FTransform> Checkpoint;
        bool bRespawnPending = false;
        bool bCompleted = false;
    };

    TMap<TWeakObjectPtr<const APlayerController>, FDPlayerProgress> Players;
};
