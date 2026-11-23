#pragma once

#include "CoreKitWeapons/CoreKitWeaponTypes.h"
#include "GameFramework/Actor.h"
#include "CoreKitWeaponProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UCoreKitWeaponDefinition;
class UCoreKitWeaponsComponent;

/** A pooled visual actor. Explicit sweeps own movement and each launch has its own lifetime and hit history. */
UCLASS(NotBlueprintable)
class ACoreKitWeaponProjectile : public AActor
{
    GENERATED_BODY()

public:
    ACoreKitWeaponProjectile();
    void Launch(UCoreKitWeaponsComponent& Source, UCoreKitWeaponDefinition& InDefinition,
        const FCoreKitWeaponAim& Aim, const FCoreKitWeaponImpact& Impact);
    bool IsLeased() const { return bLeased; }
    void Tick(float DeltaSeconds) override;

private:
    void Release();
    AActor* FindRicochetTarget(const FVector& Point) const;

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<UCoreKitWeaponDefinition> Definition;
    TWeakObjectPtr<UCoreKitWeaponsComponent> Weapons;
    UPROPERTY() FCoreKitWeaponImpact Snapshot;
    FVector Direction = FVector::ForwardVector;
    FVector PlaneNormal = FVector::ZeroVector;
    TSet<TWeakObjectPtr<AActor>> HitActors;
    float Remaining = 0.f;
    int32 Penetrations = 0;
    int32 Ricochets = 0;
    bool bLeased = false;
};
