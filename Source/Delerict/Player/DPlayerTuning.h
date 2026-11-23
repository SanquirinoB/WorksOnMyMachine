#pragma once

#include "Engine/CollisionProfile.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "DPlayerTuning.generated.h"

class UDWeaponImpulseTuning;

UCLASS(BlueprintType)
class DELERICT_API UDPlayerTuning : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Gravity acceleration reported by Mover in cm/s^2. The zero vector gives the player zero gravity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    FVector GravityAcceleration = FVector::ZeroVector;

    /** Fixed capsule world rotation in degrees. Also sets the initial mesh orientation before the first magnetic attachment. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    FRotator LockedRotation = FRotator::ZeroRotator;

    /** Nonzero normal of the movement plane through the spawn location. Positive Y limits movement to world X and Z. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    FVector MovementPlaneNormal = FVector::RightVector;

    /** Keys that request travel toward negative world X while attached. An empty list leaves this direction unbound on the keyboard. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    TArray<FKey> MoveLeftKeys = { EKeys::A, EKeys::Left };

    /** Keys that request travel toward positive world X while attached. An empty list leaves this direction unbound on the keyboard. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    TArray<FKey> MoveRightKeys = { EKeys::D, EKeys::Right };

    /** Keys that request travel toward positive world Z while attached. An empty list leaves this direction unbound on the keyboard. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    TArray<FKey> MoveUpKeys = { EKeys::W, EKeys::Up };

    /** Keys that request travel toward negative world Z while attached. An empty list leaves this direction unbound on the keyboard. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    TArray<FKey> MoveDownKeys = { EKeys::S, EKeys::Down };

    /** Two-dimensional gamepad stick that requests travel along a magnetic face. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    FKey MoveStickKey = EKeys::Gamepad_Left2D;

    /** Move-stick magnitude below which input is ignored, from 0 to 1. Zero removes the dead zone. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement", meta = (ClampMin = "0", ClampMax = "1"))
    float MoveStickDeadZone = .2f;

    /** Buttons held to travel at running speed while attached. An empty list leaves Run unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    TArray<FKey> RunKeys = { EKeys::LeftShift, EKeys::Gamepad_LeftThumbstick };

    /** Two-dimensional gamepad stick that sets the aim direction used by jumps and throws. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Aim")
    FKey AimStickKey = EKeys::Gamepad_Right2D;

    /** Aim-stick magnitude below which the last valid aim is retained, from 0 to 1. Zero removes the dead zone. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Aim", meta = (ClampMin = "0", ClampMax = "1"))
    float AimStickDeadZone = .2f;

    /** Nonzero world direction used until the player first aims. The default points along positive X. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Aim")
    FVector InitialAimDirection = FVector::ForwardVector;

    /** Interact buttons that grab a reachable throwable. An empty list leaves Interact unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TArray<FKey> GrabKeys = { EKeys::F, EKeys::Gamepad_LeftShoulder };

    /** Maximum gap in cm from the player capsule to a throwable that the player can grab. Zero requires contact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws", meta = (ClampMin = "0", Units = "cm"))
    float GrabDistance = 60.f;

    /** Buttons that cycle through throwable selections. An empty list leaves selection cycling unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TArray<FKey> SelectThrowableKeys = { EKeys::Tab, EKeys::Gamepad_DPad_Up };

    /** Direct selection keys by throwable type. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TMap<FName, FKey> ThrowableHotkeys = {
        { TEXT("WeightedPlate"), EKeys::One },
        { TEXT("CompletoItaliano"), EKeys::Two },
        { TEXT("GrapplingHook"), EKeys::Three },
        { TEXT("PortableBlackHoleDevice"), EKeys::Four },
    };

    /** Buttons that activate the most recently deployed item that supports remote activation. An empty list leaves activation unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TArray<FKey> ActivateThrowableKeys = { EKeys::Q };

    /** Buttons held to charge the selected item and released to throw it. An empty list leaves Throw unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TArray<FKey> ThrowKeys = { EKeys::RightMouseButton, EKeys::Gamepad_RightShoulder };

    /** Buttons held while attached to charge a jump and released to launch it. An empty list leaves Jump unbound. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TArray<FKey> JumpKeys = { EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom };

    /** Buttons held while floating to latch onto steel. Releasing them after capture keeps the pawn attached. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Actions")
    TArray<FKey> MagnetHoldKeys = { EKeys::E };

    /** Radius in cm of the capsule used for collision sweeps and magnetic distance checks. Use a positive value. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (ClampMin = "0.1", Units = "cm"))
    float CapsuleRadius = 34.f;

    /** Distance in cm from the capsule center to either end, including the cap. Use at least the capsule radius. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (ClampMin = "0.1", Units = "cm"))
    float CapsuleHalfHeight = 88.f;

    /** Base collision profile applied to the player capsule before adding the weapon trace response. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision")
    FCollisionProfileName CapsuleCollisionProfile = FCollisionProfileName(TEXT("Pawn"));

    /** Weapon impulse settings for this player. Assign DA_DWeaponImpulseTuning or another weapon impulse asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    TObjectPtr<UDWeaponImpulseTuning> WeaponImpulseTuning = nullptr;

    /** Character mass in kg with an empty inventory. Use a positive value. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mass", meta = (ClampMin = "0.01", Units = "kg"))
    float PlayerMass = 90.f;

    /** Mass in kg by throwable type. Give every throwable used in a level a positive entry. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mass")
    TMap<FName, float> ThrowableMassByType = {
        { TEXT("WeightedPlate"), 5.f },
        { TEXT("CompletoItaliano"), .1f },
        { TEXT("GrapplingHook"), 1.f },
        { TEXT("RemoteGrenade"), .5f },
        { TEXT("PortableBlackHoleDevice"), 2.f },
        { TEXT("Heavy"), 8.f },
    };

    /** Maximum carried quantity by type. Types without an entry can be carried in any quantity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws", meta = (ClampMin = "0"))
    TMap<FName, int32> ThrowableCarryLimits = {
        { TEXT("GrapplingHook"), 1 },
        { TEXT("PortableBlackHoleDevice"), 1 },
    };

    /** Throwable type used for the starting plate stack. Its mass comes from Throwable Mass by Type. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws")
    FName StartingPlateType = TEXT("WeightedPlate");

    /** Number of plates granted at spawn. Zero starts the player without plates. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws", meta = (ClampMin = "0"))
    int32 StartingPlateCount = 3;

    /** Hold time in seconds to reach full throw strength. Zero reaches full strength immediately. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws", meta = (ClampMin = "0", Units = "s"))
    float ThrowChargeTime = .65f;

    /** Relative player-to-item speed in cm/s after a tap throw in free flight. Zero adds no separation speed on a tap. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws", meta = (ClampMin = "0", Units = "cm/s"))
    float MinThrowSeparationSpeed = 400.f;

    /** Relative player-to-item speed in cm/s after a fully charged throw in free flight. Use at least the tap speed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Throws", meta = (ClampMin = "0", Units = "cm/s"))
    float MaxThrowSeparationSpeed = 1600.f;

    /** Maximum gap in cm between the capsule and a magnetic face at which capture can occur. Zero requires contact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0", Units = "cm"))
    float CaptureDistance = 60.f;

    /** Gap in cm maintained between the capsule and the attached face. Zero puts the capsule against the face. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0", Units = "cm"))
    float ContactClearance = 2.f;

    /** Target speed in cm/s at full tangential input when walking on a magnetic face. Zero sets the walk target to rest. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0", Units = "cm/s"))
    float SurfaceWalkSpeed = 200.f;

    /** Target speed in cm/s at full tangential input while holding Run on a magnetic face. Zero sets the run target to rest. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0", Units = "cm/s"))
    float SurfaceRunSpeed = 350.f;

    /** Rate in cm/s^2 at which input accelerates travel along a magnetic face. Zero disables input acceleration. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0"))
    float SurfaceAcceleration = 1500.f;

    /** Braking rate in cm/s^2 when attached travel exceeds the requested speed. Zero disables speed-based braking. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0"))
    float SurfaceBraking = 1500.f;

    /** Hold time in seconds to reach full jump impulse. Zero reaches full strength immediately. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Jump", meta = (ClampMin = "0", Units = "s"))
    float JumpChargeTime = .70f;

    /** Impulse in kg*cm/s added by a tap jump along the permitted launch direction. Zero adds no impulse on a tap. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Jump", meta = (ClampMin = "0"))
    float MinJumpImpulseMagnitude = 8000.f;

    /** Impulse in kg*cm/s added by a fully charged jump along the permitted launch direction. Use at least the tap impulse. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Jump", meta = (ClampMin = "0"))
    float MaxJumpImpulseMagnitude = 16000.f;

    /** Minimum outward launch angle in degrees from the current surface tangent. At 90 degrees all jumps launch along the normal. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Jump", meta = (ClampMin = "20", ClampMax = "90", Units = "deg"))
    float MinJumpAngleDegrees = 20.f;

    /** Gap in cm the capsule must clear before a departed face can capture it again during the same hold. Set it above Capture Distance. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magnetic Surface", meta = (ClampMin = "0", Units = "cm"))
    float RecaptureDistance = 80.f;

    /** Time in seconds multiplied by the player's X velocity to set camera lead. Zero removes look-ahead. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0", Units = "s"))
    float CameraLookAheadTime = .18f;

    /** Maximum horizontal camera lead in cm in either direction. Zero removes look-ahead. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0", Units = "cm"))
    float CameraMaxLookAhead = 90.f;
};
