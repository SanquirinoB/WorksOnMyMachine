#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "CoreKitWeaponTypes.generated.h"

class AActor;
class ACoreKitWeapon;

UENUM(BlueprintType)
enum class ECoreKitWeaponFireMode : uint8 { Single, Automatic, Burst };

UENUM(BlueprintType)
enum class ECoreKitWeaponDelivery : uint8 { Projectile, Hitscan, Melee };

UENUM(BlueprintType)
enum class ECoreKitWeaponReloadMode : uint8 { Magazine, PerRound, Disabled };

UENUM(BlueprintType)
enum class ECoreKitWeaponImpactKind : uint8 { Direct, Area };

/** Aim is supplied by the game. A zero normal permits free 3D aiming. */
USTRUCT(BlueprintType)
struct COREKITWEAPONS_API FCoreKitWeaponAim
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") FVector Origin = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") FVector Direction = FVector::ForwardVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") FVector PlaneNormal = FVector::ZeroVector;
};

/** Data copied at firing time. A game can handle team, health, armor and reaction in its receiver. */
USTRUCT(BlueprintType)
struct COREKITWEAPONS_API FCoreKitWeaponImpact
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Weapon") TObjectPtr<AActor> InstigatorActor = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") FName WeaponId;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") FGameplayTagContainer DamageTags;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") ECoreKitWeaponImpactKind Kind = ECoreKitWeaponImpactKind::Direct;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") FVector Origin = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") FVector Direction = FVector::ForwardVector;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") FHitResult Hit;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") float Damage = 0.f;
};

USTRUCT(BlueprintType)
struct COREKITWEAPONS_API FCoreKitWeaponState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Weapon") int32 Magazine = 0;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") int32 Reserve = 0;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") int32 Capacity = 0;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") bool bUnlimitedReserve = false;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") bool bReloading = false;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") float ReloadProgress = 0.f;
};

USTRUCT(BlueprintType)
struct COREKITWEAPONS_API FCoreKitWeaponMeleeStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0.01")) float Duration = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0")) float HitStart = .1f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0")) float HitEnd = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0")) float BufferOpen = .24f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(ClampMin="0")) float DamageScale = 1.f;
};
