#pragma once

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DMoverComponent.generated.h"

UCLASS()
class UDMoverComponent : public UCharacterMoverComponent
{
    GENERATED_BODY()

public:
    UDMoverComponent();
    void PostInitProperties() override;
};
