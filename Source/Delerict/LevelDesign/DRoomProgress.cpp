#include "DRoomProgress.h"

#include "../Frontend/DMenu.h"
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
    if (Progress.bRespawnPending || Player.GetPawn() != &Pawn)
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
                FDPlayerProgress& State = Players.FindChecked(Current);
                if (!Previous.IsValid() || Current->GetPawn() != Previous.Get())
                {
                    State.bRespawnPending = false;
                    return;
                }
                if (GetWorld()->GetNetMode() == NM_Standalone && UDMenu::OpenDeath(*Current, State.Checkpoint.IsSet()))
                {
                    return;
                }
                // Worlds without a local menu keep the existing automatic recovery.
                if (!Retry(*Current))
                {
                    Players.FindChecked(Current).bRespawnPending = false;
                }
            }
        }));
}

bool UDRoomProgress::Retry(APlayerController& Player)
{
    FDPlayerProgress* Progress = Players.Find(&Player);
    APawn* PreviousPawn = Player.GetPawn();
    AGameModeBase* Mode = GetWorld()->GetAuthGameMode();
    if (!Progress || !Progress->bRespawnPending || !PreviousPawn || !Mode)
    {
        return false;
    }
    // The previous capsule can still occupy the start if the player died there.
    const bool bWasCollidable = PreviousPawn->GetActorEnableCollision();
    PreviousPawn->SetActorEnableCollision(false);
    APawn* Replacement = nullptr;
    if (Progress->Checkpoint.IsSet())
    {
        Replacement = Mode->SpawnDefaultPawnAtTransform(&Player, Progress->Checkpoint.GetValue());
    }
    if (!Replacement)
    {
        if (AActor* Start = Mode->FindPlayerStart(&Player))
        {
            Replacement = Mode->SpawnDefaultPawnFor(&Player, Start);
        }
    }
    if (!Replacement)
    {
        PreviousPawn->SetActorEnableCollision(bWasCollidable);
        return false;
    }
    Player.Possess(Replacement);
    if (Player.GetPawn() != Replacement)
    {
        Replacement->Destroy();
        PreviousPawn->SetActorEnableCollision(bWasCollidable);
        return false;
    }
    PreviousPawn->Destroy();
    Players.FindChecked(&Player).bRespawnPending = false;
    return true;
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
