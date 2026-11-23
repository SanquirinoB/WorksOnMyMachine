#pragma once

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "WorksOnMyMachineMoverComponent.generated.h"

UCLASS()
class UWorksOnMyMachineMoverComponent : public UCharacterMoverComponent
{
    GENERATED_BODY()

public:
    UWorksOnMyMachineMoverComponent();
    void PostInitProperties() override;
};
