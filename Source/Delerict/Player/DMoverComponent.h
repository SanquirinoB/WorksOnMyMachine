#pragma once

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DMoverComponent.generated.h"

class UDMagneticSurfaceMode;
class USceneComponent;

UCLASS()
class UDMoverComponent : public UCharacterMoverComponent
{
    GENERATED_BODY()

public:
    UDMoverComponent();
    void PostInitProperties() override;
    void InitializeComponent() override;
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;

    bool IsMagneticallyAttached() const;
    float GetSurfaceTravelSpeed() const;
    void UpdateMagneticVisuals(USceneComponent& Mesh, USceneComponent& Placeholder);

#if DELERICT_WITH_GAMEPLAY_DEBUG
    TOptional<FHitResult> TakeFlightImpact();

protected:
    void OnHandleImpact(const FMoverOnImpactParams& Impact) override;

private:
    TOptional<FHitResult> FlightImpact;
#endif

private:
    UDMagneticSurfaceMode* GetMagneticMode() const;
    TOptional<FTransform> MeshRestPose;
    TOptional<FTransform> PlaceholderRestPose;
};
