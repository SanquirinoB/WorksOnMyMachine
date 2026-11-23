#pragma once

#include "Components/SceneComponent.h"
#include "DSteelSurfaceComponent.generated.h"

class UCapsuleComponent;

/** A rectangular magnetic face. Attach it directly to the blocking primitive for the steel. */
UCLASS(ClassGroup = (Delerict), meta = (BlueprintSpawnableComponent))
class UDSteelSurfaceComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    bool CanSupport(const UCapsuleComponent& Capsule) const;
    bool ContainsProjection(const FVector& Point, double Tolerance = 0.) const;
    FVector ClosestPoint(const FVector& Point) const;
    FVector LimitTravel(const FVector& Point, const FVector& Delta) const;

    /** Half width and depth in local X and Y, in cm. Local +Z points out of the face. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steel Face", meta = (ClampMin = "0.1", Units = "cm"))
    FVector2D HalfExtent = FVector2D(150., 80.);

    /** Disabling the face releases attached pawns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steel Face")
    bool bMagnetic = true;
};
