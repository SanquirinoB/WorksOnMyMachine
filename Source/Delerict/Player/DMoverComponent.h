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
    void InitializeComponent() override;

#if DELERICT_WITH_GAMEPLAY_DEBUG
    TOptional<FHitResult> TakeFlightImpact();

protected:
    void OnHandleImpact(const FMoverOnImpactParams& Impact) override;

private:
    TOptional<FHitResult> FlightImpact;
#endif
};
