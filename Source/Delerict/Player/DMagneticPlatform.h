#pragma once

#include "GameFramework/Actor.h"
#include "DMagneticPlatform.generated.h"

class UBoxComponent;
class UDSteelSurfaceComponent;
class UStaticMeshComponent;

/** A movable steel panel with its magnetic face on the local +Z side. */
UCLASS(Blueprintable)
class ADMagneticPlatform : public AActor
{
    GENERATED_BODY()

public:
    ADMagneticPlatform();
    void OnConstruction(const FTransform& Transform) override;

private:
    /** Half dimensions of the blocking and visible panel, in cm. */
    UPROPERTY(EditAnywhere, Category = "Steel", meta = (ClampMin = "0.1", Units = "cm"))
    FVector HalfExtent = FVector(150., 80., 12.);

    UPROPERTY(VisibleAnywhere, Category = "Steel")
    TObjectPtr<UBoxComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Steel")
    TObjectPtr<UDSteelSurfaceComponent> SteelFace;

    UPROPERTY(VisibleAnywhere, Category = "Steel")
    TObjectPtr<UStaticMeshComponent> Mesh;
};
