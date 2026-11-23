#pragma once

#include "DThrowable.h"
#include "DDeployableGadgets.generated.h"

class UPointLightComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable)
class ADGrapplingHook : public ADThrowable
{
    GENERATED_BODY()

public:
    ADGrapplingHook();

    void Tick(float DeltaSeconds) override;
    void AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal) override;

protected:
    void ResetDeployment() override;
    void OnDeployed(ADPawn* Pawn) override;
    void OnDeployedImpact(const FHitResult& Hit) override;
    void OnRemoteActivationRequested(ADPawn& Pawn) override;

    /** Maximum player speed toward an attached hook after an activation, in cm/s. Zero disables pulling. */
    UPROPERTY(EditAnywhere, Category = "Grapple", meta = (ClampMin = "0", Units = "cm/s"))
    float PullSpeed = 560.f;

    /** Speed of the free hook when reeling it in, in cm/s. Zero disables recall. */
    UPROPERTY(EditAnywhere, Category = "Grapple", meta = (ClampMin = "0", Units = "cm/s"))
    float RecallSpeed = 1200.f;

private:
    UPROPERTY(VisibleAnywhere, Category = "Grapple")
    TObjectPtr<UStaticMeshComponent> Tether;

    TWeakObjectPtr<UPrimitiveComponent> Anchor;
    bool bAttachedToAnchor = false;
    bool bReelingIn = false;

    void ClearAnchor();
    bool UpdateRecall(ADPawn& Pawn, float DeltaSeconds);
};

UCLASS(Blueprintable)
class ADRemoteGrenade : public ADThrowable
{
    GENERATED_BODY()

public:
    ADRemoteGrenade();

protected:
    void OnRemoteActivationRequested(ADPawn& Pawn) override;

    /** Radius of the gameplay damage burst in cm. Zero disables area damage. */
    UPROPERTY(EditAnywhere, Category = "Grenade", meta = (ClampMin = "0", Units = "cm"))
    float BurstRadius = 230.f;

    /** Peak damage at the burst center. Zero leaves the visual flash only. */
    UPROPERTY(EditAnywhere, Category = "Grenade", meta = (ClampMin = "0"))
    float BurstDamage = 65.f;

private:
    bool bDetonated = false;
};

UCLASS(Blueprintable)
class ADPortableBlackHoleDevice : public ADThrowable
{
    GENERATED_BODY()

public:
    ADPortableBlackHoleDevice();

    void Tick(float DeltaSeconds) override;

protected:
    void BeginPlay() override;
    void ResetDeployment() override;
    void OnRemoteActivationRequested(ADPawn& Pawn) override;

    /** Range of the active attraction field in cm. Zero disables attraction and consumption. */
    UPROPERTY(EditAnywhere, Category = "Black Hole", meta = (ClampMin = "0", Units = "cm"))
    float AttractionRadius = 630.f;

    /** Peak attraction acceleration at the core in cm/s², with linear falloff to the field edge. */
    UPROPERTY(EditAnywhere, Category = "Black Hole", meta = (ClampMin = "0"))
    float AttractionAcceleration = 900.f;

    /** Radius of the consumption core in cm. Zero disables consumption and rupture. */
    UPROPERTY(EditAnywhere, Category = "Black Hole", meta = (ClampMin = "0", Units = "cm"))
    float CoreRadius = 38.f;

    /** Maximum range of the rupture burst in cm. Zero disables rupture damage and pushback. */
    UPROPERTY(EditAnywhere, Category = "Black Hole", meta = (ClampMin = "0", Units = "cm"))
    float RuptureRadius = 420.f;

    /** Peak damage at the rupture center. Zero disables rupture damage. */
    UPROPERTY(EditAnywhere, Category = "Black Hole", meta = (ClampMin = "0"))
    float RuptureDamage = 100.f;

    /** Peak outward change in speed from the rupture in cm/s. Zero disables pushback. */
    UPROPERTY(EditAnywhere, Category = "Black Hole", meta = (ClampMin = "0", Units = "cm/s"))
    float RupturePushSpeed = 380.f;

private:
    UPROPERTY(VisibleAnywhere, Category = "Black Hole")
    TObjectPtr<UStaticMeshComponent> FieldCore;

    UPROPERTY(VisibleAnywhere, Category = "Black Hole")
    TObjectPtr<UStaticMeshComponent> FieldOrbit;

    UPROPERTY(VisibleAnywhere, Category = "Black Hole")
    TObjectPtr<UStaticMeshComponent> FieldOrbitOpposite;

    UPROPERTY(VisibleAnywhere, Category = "Black Hole")
    TObjectPtr<UPointLightComponent> FieldLight;

    bool bFieldActive = false;

    void SetFieldActive(bool bActive);
    void Rupture(AActor& ConsumedActor);
};
