#include "DRoomProgress.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

bool UDRoomProgress::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UDRoomProgress::SetCheckpoint(APlayerController& Player, const FTransform& Transform)
{
    FDPlayerProgress& Progress = Players.FindOrAdd(&Player);
    if (Progress.bRespawnPending || (Progress.Checkpoint.IsSet() && Progress.Checkpoint->Equals(Transform)))
    {
        return false;
    }
    Progress.Checkpoint = Transform;
    return true;
}

void UDRoomProgress::RequestRespawn(APlayerController& Player, APawn& Pawn)
{
    FDPlayerProgress& Progress = Players.FindOrAdd(&Player);
    if (Progress.bRespawnPending)
    {
        return;
    }
    Progress.bRespawnPending = true;

    // Overlaps can fire in a movement update. Replace the pawn after that update.
    GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
        [this, Controller = TWeakObjectPtr<APlayerController>(&Player), Previous = TWeakObjectPtr<APawn>(&Pawn)]
        {
            if (APlayerController* Current = Controller.Get())
            {
                Players.FindChecked(Current).bRespawnPending = false;
                if (APawn* OldPawn = Previous.Get(); OldPawn && Current->GetPawn() == OldPawn)
                {
                    Respawn(*Current, *OldPawn);
                }
            }
        }));
}

void UDRoomProgress::Respawn(APlayerController& Player, APawn& PreviousPawn)
{
    AGameModeBase* Mode = GetWorld()->GetAuthGameMode();
    if (!Mode)
    {
        return;
    }
    const FDPlayerProgress& Progress = Players.FindChecked(&Player);
    APawn* Replacement = nullptr;
    if (Progress.Checkpoint.IsSet())
    {
        Replacement = Mode->SpawnDefaultPawnAtTransform(&Player, Progress.Checkpoint.GetValue());
    }
    if (!Replacement)
    {
        if (AActor* Start = Mode->FindPlayerStart(&Player))
        {
            Replacement = Mode->SpawnDefaultPawnFor(&Player, Start);
        }
    }
    if (Replacement)
    {
        Player.Possess(Replacement);
        if (Player.GetPawn() == Replacement)
        {
            PreviousPawn.Destroy();
        }
        else
        {
            Replacement->Destroy();
        }
    }
}

bool UDRoomProgress::CompleteRoom(APlayerController& Player)
{
    FDPlayerProgress& Progress = Players.FindOrAdd(&Player);
    if (Progress.bRespawnPending || Progress.bCompleted)
    {
        return false;
    }
    Progress.bCompleted = true;
    return true;
}

bool UDRoomProgress::HasCompletedRoom(const APlayerController& Player) const
{
    const FDPlayerProgress* Progress = Players.Find(&Player);
    return Progress && Progress->bCompleted;
}
