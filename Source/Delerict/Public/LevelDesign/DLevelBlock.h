#pragma once

#include "GameFramework/Actor.h"
#include "DLevelBlock.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;

/** A rectangular platforming piece. Its origin is the center of its bottom face. */
UCLASS(Blueprintable)
class DELERICT_API ADLevelBlock : public AActor
{
    GENERATED_BODY()

public:
    ADLevelBlock();
    void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Piece", meta = (ClampMin = "1", Units = "cm"))
    FVector Dimensions = FVector(400, 300, 40);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Piece")
    TObjectPtr<UMaterialInterface> Finish;

private:
    UPROPERTY(VisibleAnywhere, Category = "Piece")
    TObjectPtr<USceneComponent> Pivot;

    UPROPERTY(VisibleAnywhere, Category = "Piece")
    TObjectPtr<UBoxComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Piece")
    TObjectPtr<UStaticMeshComponent> Visual;
};
