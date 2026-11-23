#pragma once

#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "MovementMode.h"
#include "DMagneticSurfaceMode.generated.h"

class ADPawn;
class UCapsuleComponent;
class UDSteelSurfaceComponent;
struct FPlanarConstraint;
enum class EDChargePhase : uint8;

UCLASS()
class UDFlyingMode : public UFlyingMode
{
    GENERATED_BODY()

    void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;
};

/** Swept tangential travel and platform carry while the magnetic input is held. */
UCLASS()
class UDMagneticSurfaceMode : public UBaseMovementMode
{
    GENERATED_BODY()

public:
    UDMagneticSurfaceMode();
    static const FName ModeName;

    bool TryCapture(const FSimulationTickParams& Params, FMoverTickEndData& OutputState);
    void HandleJumpCharge(EDChargePhase Phase, const FVector& Aim);
    bool IsAttached() const { return Surface.IsValid(); }
    FVector GetSurfaceNormal() const { return SurfaceNormal; }
    FVector GetFacingTangent() const { return SurfaceTangent * FacingSign; }
    float GetRelativeSpeed() const { return FMath::Abs(RelativeSpeed); }

    void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState,
        const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
    void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

private:
    struct FContact
    {
        FVector Point = FVector::ZeroVector;
        FVector Normal = FVector::UpVector;
        FVector Tangent = FVector::ForwardVector;
        double CapsuleSupport = 0.;
        double Gap = 0.;
    };

    static bool SampleFace(const UDSteelSurfaceComponent& Face, const UCapsuleComponent& Capsule,
        const FPlanarConstraint& Plane, const FVector& Location, FContact& Contact);
    static bool SweepToContact(const FSimulationTickParams& Params, const FVector& Target);
    static void WriteSyncState(const FSimulationTickParams& Params, FMoverTickEndData& OutputState,
        const FVector& Velocity);

    ADPawn* GetPawn() const;
    void Depart(const FSimulationTickParams& Params, FMoverTickEndData& OutputState, const FVector& Velocity,
        const TCHAR* Reason, bool bRefundTime = true);
    void RearmFace(const UCapsuleComponent& Capsule, double RecaptureDistance);

    TWeakObjectPtr<UDSteelSurfaceComponent> Surface;
    TWeakObjectPtr<UDSteelSurfaceComponent> DepartedFace;
    FVector ContactLocal = FVector::ZeroVector;
    FVector SurfaceNormal = FVector::UpVector;
    FVector SurfaceTangent = FVector::ForwardVector;
    FVector LastCarryVelocity = FVector::ZeroVector;
    FVector LastWorldVelocity = FVector::ZeroVector;
    double LastDepartureTimeMs = -1.;
    float RelativeSpeed = 0.f;
    float FacingSign = 1.f;
    TOptional<double> JumpStartedAt;
    bool bJumpDeparturePending = false;
};
