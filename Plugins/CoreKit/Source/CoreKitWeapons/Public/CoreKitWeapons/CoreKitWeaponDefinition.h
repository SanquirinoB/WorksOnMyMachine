#pragma once

#include "CoreKitWeapons/CoreKitWeaponTypes.h"
#include "Engine/DataAsset.h"
#include "CoreKitWeaponDefinition.generated.h"

class UStaticMesh;
class USoundBase;

/** Shared, authored configuration. The weapon instance owns current ammunition and operation state. */
UCLASS(BlueprintType)
class COREKITWEAPONS_API UCoreKitWeaponDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon") FName WeaponId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon") FName OverlayId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon") ECoreKitWeaponDelivery Delivery = ECoreKitWeaponDelivery::Projectile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon") ECoreKitWeaponFireMode FireMode = ECoreKitWeaponFireMode::Single;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.01")) float FireInterval = .18f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0")) float EquipSeconds = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="1", ClampMax="32")) int32 BurstCount = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="1", ClampMax="32")) int32 Pellets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0", ClampMax="45")) float SpreadDegrees = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="1")) float Range = 4000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="1")) int32 MagazineSize = 12;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0")) int32 InitialReserve = 36;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="1")) int32 AmmoPerShot = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo") bool bUnlimitedReserve = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo") bool bAutoReload = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo") ECoreKitWeaponReloadMode ReloadMode = ECoreKitWeaponReloadMode::Magazine;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ammo", meta=(ClampMin="0.01")) float ReloadSeconds = .85f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0")) float Damage = 15.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact") FGameplayTagContainer DamageTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0")) float BlastRadius = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0")) float BlastDamage = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="1")) float ProjectileSpeed = 5000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.01")) float ProjectileLifetime = 1.8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0.1")) float CollisionRadius = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="1", ClampMax="128")) int32 PoolCapacity = 24;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0", ClampMax="16")) int32 Penetrations = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="0", ClampMax="8")) int32 Ricochets = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(ClampMin="1")) float RicochetRadius = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee") TArray<FCoreKitWeaponMeleeStep> MeleeSteps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee", meta=(ClampMin="1")) float MeleeRadius = 28.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMesh> WeaponMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMesh> ProjectileMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FVector ProjectileVisualScale = FVector(.15);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<USoundBase> FireSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") TObjectPtr<USoundBase> ReloadSound;

    bool IsUsable() const;
};
