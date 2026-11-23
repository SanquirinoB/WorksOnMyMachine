#include "DCheckpointParticipant.h"

#include "DRoomProgress.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UDCheckpointParticipant::UDCheckpointParticipant()
{
    // The gameplay GameMode exists exactly once in each saved world.
    PersistenceId = FGuid(0xAC4D3706, 0x7B384166, 0xB00EF71C, 0x58D70D73);
}

bool UDCheckpointParticipant::CaptureSaveState_Implementation(FCoreKitParticipantState& OutState)
{
    const auto* Progress = GetWorld()->GetSubsystem<UDRoomProgress>();
    const auto* Player = GetWorld()->GetFirstPlayerController();
    if (!Progress || !Player) { return false; }

    FDCheckpointState Snapshot;
    if (const auto Checkpoint = Progress->GetCheckpoint(*Player))
    {
        Snapshot.bActivated = true;
        Snapshot.RespawnTransform = Checkpoint.GetValue();
    }
    OutState.Version = 1;
    OutState.Payload.InitializeAs<FDCheckpointState>(Snapshot);
    return true;
}

bool UDCheckpointParticipant::CanRestoreSaveState_Implementation(const FCoreKitParticipantState& State)
{
    const auto* Snapshot = State.Payload.GetPtr<FDCheckpointState>();
    const auto* Player = GetWorld()->GetFirstPlayerController();
    return State.Version == 1 && Snapshot && GetWorld()->GetSubsystem<UDRoomProgress>() &&
        (!Snapshot->bActivated || (Player && Player->GetPawn() && Snapshot->RespawnTransform.IsValid()));
}

void UDCheckpointParticipant::ApplySaveState_Implementation(const FCoreKitParticipantState& State)
{
    const auto& Snapshot = State.Payload.Get<FDCheckpointState>();
    if (Snapshot.bActivated)
    {
        GetWorld()->GetSubsystem<UDRoomProgress>()->RestoreCheckpoint(
            *GetWorld()->GetFirstPlayerController(), Snapshot.RespawnTransform);
    }
}
