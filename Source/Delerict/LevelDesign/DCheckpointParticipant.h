#pragma once

#include "CoreKitSave/CoreKitWorldSave.h"
#include "DCheckpointParticipant.generated.h"

USTRUCT()
struct FDCheckpointState
{
    GENERATED_BODY()

    UPROPERTY()
    bool bActivated = false;

    UPROPERTY()
    FTransform RespawnTransform;
};

/** One participant per gameplay world, independent of the currently possessed pawn. */
UCLASS()
class UDCheckpointParticipant : public UCoreKitSaveParticipantComponent
{
    GENERATED_BODY()

public:
    UDCheckpointParticipant();
    bool CaptureSaveState_Implementation(FCoreKitParticipantState& OutState) override;
    bool CanRestoreSaveState_Implementation(const FCoreKitParticipantState& State) override;
    void ApplySaveState_Implementation(const FCoreKitParticipantState& State) override;
};
