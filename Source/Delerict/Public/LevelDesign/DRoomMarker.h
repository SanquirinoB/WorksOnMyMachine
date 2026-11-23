#pragma once

#include "GameFramework/Actor.h"
#include "DRoomMarker.generated.h"

class APawn;
class UBoxComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWorld;

UENUM(BlueprintType)
enum class EDRoomMarkerRole : uint8
{
    Hazard,
    Checkpoint,
    Exit
};

UCLASS(Blueprintable)
class DELERICT_API ADRoomMarker : public AActor
{
    GENERATED_BODY()

public:
    ADRoomMarker();
    void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Marker")
    EDRoomMarkerRole MarkerRole = EDRoomMarkerRole::Hazard;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker", meta = (ClampMin = "1", Units = "cm"))
    FVector Dimensions = FVector(120, 300, 80);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
    FText Feedback;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker", meta = (EditCondition = "MarkerRole == EDRoomMarkerRole::Checkpoint", EditConditionHides, Units = "cm"))
    FVector RespawnOffset = FVector(0, 0, 100);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker", meta = (EditCondition = "MarkerRole == EDRoomMarkerRole::Exit", EditConditionHides))
    TSoftObjectPtr<UWorld> Destination;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
    TObjectPtr<UMaterialInterface> Finish;

private:
    UPROPERTY(VisibleAnywhere, Category = "Marker")
    TObjectPtr<USceneComponent> Pivot;

    UPROPERTY(VisibleAnywhere, Category = "Marker")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere, Category = "Marker")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, Category = "Marker")
    TObjectPtr<UTextRenderComponent> Caption;

    UFUNCTION()
    void OnEntered(UPrimitiveComponent* Overlapped, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
        int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Marker", meta = (DisplayName = "On Marker Activated"))
    void OnMarkerActivated(APawn* PlayerPawn);
};
