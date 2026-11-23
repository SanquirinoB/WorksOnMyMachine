#include "DRoomProgress.h"

#include "../Frontend/DMenu.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

bool UDRoomProgress::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDRoomProgress::Deinitialize()
{
    if (auto* Frontend = UGameInstance::GetSubsystem<UCoreKitFrontendSubsystem>(GetWorld()->GetGameInstance()))
    {
        Frontend->OnGameArrived.RemoveDynamic(this, &ThisClass::OnGameArrived);
        Frontend->OnCheckpointSaved.RemoveDynamic(this, &ThisClass::OnCheckpointSaved);
    }
    Super::Deinitialize();
}

bool UDRoomProgress::SetCheckpoint(APlayerController& Player, const FTransform& Transform)
{
    FDPlayerProgress& Progress = Players.FindOrAdd(&Player);
    if (Progress.bRespawnPending || (!bSaveFailed && Progress.Checkpoint.IsSet() && Progress.Checkpoint->Equals(Transform)))
    {
        return false;
    }
    Progress.Checkpoint = Transform;
    if (GetWorld()->GetNetMode() == NM_Standalone && Player.IsLocalController())
    {
        SaveCheckpoint();
    }
    return true;
}

TOptional<FTransform> UDRoomProgress::GetCheckpoint(const APlayerController& Player) const
{
    const auto* Progress = Players.Find(&Player);
    return Progress ? Progress->Checkpoint : TOptional<FTransform>();
}

void UDRoomProgress::SaveCheckpoint()
{
    auto* Frontend = GetWorld()->GetGameInstance()->GetSubsystem<UCoreKitFrontendSubsystem>();
    if (!Frontend) { return; }
    if (Frontend->GetState() == ECoreKitFrontendState::Traveling)
    {
        // A start marker can overlap the pawn before New Game or Continue finishes arriving.
        Frontend->OnGameArrived.AddUniqueDynamic(this, &ThisClass::OnGameArrived);
        return;
    }
    Frontend->OnCheckpointSaved.AddUniqueDynamic(this, &ThisClass::OnCheckpointSaved);
    if (!Frontend->SaveCheckpoint(TEXT("checkpoint"), true))
    {
        OnCheckpointSaved(false);
    }
}

void UDRoomProgress::OnGameArrived(bool bContinuing)
{
    auto* Frontend = GetWorld()->GetGameInstance()->GetSubsystem<UCoreKitFrontendSubsystem>();
    Frontend->OnGameArrived.RemoveDynamic(this, &ThisClass::OnGameArrived);
    if (!bContinuing) { SaveCheckpoint(); }
}

void UDRoomProgress::OnCheckpointSaved(bool bSucceeded)
{
    bSaveFailed = !bSucceeded;
    if (bSaveFailed)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not persist the Delerict checkpoint."));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
                NSLOCTEXT("Delerict", "CheckpointSaveFailed", "Checkpoint available for this session only. Saving failed.").ToString());
        }
    }
}

void UDRoomProgress::RestoreCheckpoint(APlayerController& Player, const FTransform& Transform)
{
    Players.FindOrAdd(&Player).Checkpoint = Transform;
    if (!Respawn(Player))
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not spawn at the saved checkpoint in %s."), *GetWorld()->GetName());
    }
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
    const auto* Progress = Players.Find(&Player);
    return Progress && Progress->bRespawnPending && Respawn(Player);
}

bool UDRoomProgress::Respawn(APlayerController& Player)
{
    FDPlayerProgress& Progress = Players.FindChecked(&Player);
    APawn* PreviousPawn = Player.GetPawn();
    AGameModeBase* Mode = GetWorld()->GetAuthGameMode();
    if (!PreviousPawn || !Mode)
    {
        return false;
    }
    // The previous capsule can still occupy the start if the player died there.
    const bool bWasCollidable = PreviousPawn->GetActorEnableCollision();
    PreviousPawn->SetActorEnableCollision(false);
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
