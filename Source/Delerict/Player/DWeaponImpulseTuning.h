#pragma once

#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "DWeaponImpulseTuning.generated.h"

UENUM(BlueprintType)
enum class EDWeaponImpulseType : uint8
{
    Direct,
    Radial,
};

USTRUCT(BlueprintType)
struct FDWeaponImpulseProfile
{
    GENERATED_BODY()

    /** Impulse in kg*cm/s per accepted direct hit, along the incoming direction. Zero disables direct knockback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impulse", meta = (ClampMin = "0"))
    float DirectHitImpulseMagnitude = 24000.f;

    /** Impulse in kg*cm/s per accepted area hit, away from the blast origin. Zero disables radial knockback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impulse", meta = (ClampMin = "0"))
    float RadialHitImpulseMagnitude = 0.f;

    /** Impulse in kg*cm/s per shot, opposite the shot aim. Zero disables weapon recoil movement. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Impulse", meta = (ClampMin = "0"))
    float ShotRecoilImpulseMagnitude = 8000.f;
};

UCLASS(BlueprintType)
class DELERICT_API UDWeaponImpulseTuning : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Trace channel the player capsule blocks for incoming weapon hits. Match the equipped CoreKit weapon definitions. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    TEnumAsByte<ECollisionChannel> WeaponTraceChannel = ECC_Visibility;

    /** Impulse profile assigned to each CoreKit weapon ID. Omitted IDs use Default Weapon Impulse Type. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    TMap<FName, EDWeaponImpulseType> WeaponImpulseTypeById = {
        { TEXT("Pistol"), EDWeaponImpulseType::Direct },
        { TEXT("Rifle"), EDWeaponImpulseType::Direct },
        { TEXT("Shotgun"), EDWeaponImpulseType::Direct },
        { TEXT("Launcher"), EDWeaponImpulseType::Radial },
    };

    /** Impulse profile used when a CoreKit weapon ID has no explicit assignment. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    EDWeaponImpulseType DefaultWeaponImpulseType = EDWeaponImpulseType::Direct;

    /** Impact and shot recoil impulse magnitudes for weapons in the Direct group. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    FDWeaponImpulseProfile DirectProfile = {};

    /** Impact and shot recoil impulse magnitudes for weapons in the Radial group, including direct projectile hits. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    FDWeaponImpulseProfile RadialProfile = { 24000.f, 24000.f, 8000.f };
};
