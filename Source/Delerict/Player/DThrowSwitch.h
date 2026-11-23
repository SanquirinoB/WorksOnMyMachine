#pragma once

#include "GameFramework/Actor.h"
#include "DThrowSwitch.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDThrowSwitchActivated);

UCLASS(Blueprintable)
class ADThrowSwitch : public AActor
{
    GENERATED_BODY()

public:
    ADThrowSwitch();

    UFUNCTION(BlueprintCallable, Category = "Switch")
    void Activate();

    UFUNCTION(BlueprintPure, Category = "Switch")
    bool IsActivated() const { return bActivated; }

    /** Fired once when the switch is activated. */
    UPROPERTY(BlueprintAssignable, Category = "Switch")
    FDThrowSwitchActivated OnActivated;

private:
    UPROPERTY(VisibleAnywhere, Category = "Switch")
    TObjectPtr<UBoxComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Switch")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, Category = "Switch")
    TObjectPtr<UStaticMeshComponent> Indicator;

    bool bActivated = false;
};
