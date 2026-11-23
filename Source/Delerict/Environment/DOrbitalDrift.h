#pragma once

#include "GameFramework/Actor.h"
#include "DOrbitalDrift.generated.h"

class UCoreKitTweenSubsystem;

/** Movable pivot for non-interactive orbital scenery attached in the level. */
UCLASS()
class ADOrbitalDrift final : public AActor
{
    GENERATED_BODY()

public:
    ADOrbitalDrift();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void StartRotation(UCoreKitTweenSubsystem& Tween);
    void StartDrift(UCoreKitTweenSubsystem& Tween);

    /** Axis in the pivot's local coordinates. */
    UPROPERTY(EditAnywhere, Category = "Orbital Motion")
    FVector RotationAxis = FVector::UpVector;

    UPROPERTY(EditAnywhere, Category = "Orbital Motion", meta = (ClampMin = "0.0", DisplayName = "Rotation Speed (deg/s)"))
    float RotationSpeed = 1.0f;

    /** Maximum displacement in world coordinates on either side of the placed pivot. */
    UPROPERTY(EditAnywhere, Category = "Orbital Motion", meta = (Units = "cm"))
    FVector DriftOffset = FVector::ZeroVector;

    /** Seconds to travel from the placed pivot to one end of the drift. */
    UPROPERTY(EditAnywhere, Category = "Orbital Motion", meta = (ClampMin = "0.1", Units = "s"))
    float DriftLegSeconds = 35.0f;

    FQuat InitialRotation = FQuat::Identity;
    FVector InitialLocation = FVector::ZeroVector;
    uint64 RotationHandle = 0;
    uint64 DriftHandle = 0;
};
