#pragma once

#include "GameFramework/Actor.h"
#include "DDockingPoint.generated.h"

class APawn;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/** Supplies a world-space docking pose to the traversal system. Forward X points out of the fixture. */
UCLASS(Blueprintable)
class DELERICT_API ADDockingPoint : public AActor
{
    GENERATED_BODY()

public:
    ADDockingPoint();
    void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking", meta = (Units = "cm"))
    FVector DockingOffset = FVector(100., 0., 0.);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Docking", meta = (ClampMin = "1", Units = "cm"))
    float ApproachRadius = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Docking")
    bool bDockingEnabled = true;

    UFUNCTION(BlueprintPure, Category = "Docking")
    FTransform GetDockingPose() const;

    UFUNCTION(BlueprintCallable, Category = "Docking")
    bool TryGetDockingPose(const APawn* Pawn, FTransform& OutPose) const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Docking")
    TObjectPtr<USceneComponent> Pivot;

    UPROPERTY(VisibleAnywhere, Category = "Docking")
    TObjectPtr<USphereComponent> RangePreview;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Docking", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Fixture;
};
