#pragma once

#include "GameFramework/Actor.h"
#include "DThrowable.generated.h"

class ADPawn;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDThrowableRemoteActivation);

UCLASS(Blueprintable)
class ADThrowable : public AActor
{
    GENERATED_BODY()

public:
    ADThrowable();
    FVector GetVelocity() const override;
    FName GetThrowableType() const { return ThrowableType; }
    void SetThrowableType(FName Type) { ThrowableType = Type; }
    float GetCollisionRadius() const;
    virtual bool CanGrab() const;
    virtual bool IsReusableAfterPickup() const { return false; }
    bool IsHeldBy(const ADPawn& Pawn) const;
    bool TryHold(ADPawn& Pawn);
    bool TryReturnToOwner();
    void Launch(const FVector& Location, const FVector& Velocity, const FVector& PlaneNormal, ADPawn* Thrower);
    bool TryActivate(ADPawn& Pawn);
    virtual void AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal);

    /** Enables remote activation while this item is deployed. Weighted plates use their collision to trigger switches. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throwable")
    bool bRemoteActivatable = false;

    /** Fired when the player who launched this item requests remote activation. */
    UPROPERTY(BlueprintAssignable, Category = "Throwable")
    FDThrowableRemoteActivation OnRemoteActivated;

protected:
    void BeginPlay() override;

    ADPawn* GetDeployingPawn() const;
    virtual void ResetDeployment() {}
    virtual void OnDeployed(ADPawn* Pawn) {}
    virtual void OnDeployedImpact(const FHitResult& Hit) {}
    virtual void OnRemoteActivationRequested(ADPawn& Pawn) {}

    /** Item type used to look up mass in the player's tuning asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throwable")
    FName ThrowableType = TEXT("WeightedPlate");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, Category = "Throwable")
    TObjectPtr<UProjectileMovementComponent> Movement;

private:
    TWeakObjectPtr<ADPawn> Holder;
    TWeakObjectPtr<ADPawn> LastThrower;

    UFUNCTION()
    void OnBounce(const FHitResult& Hit, const FVector& ImpactVelocity);
};
