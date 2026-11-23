#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "GameplayTagContainer.h"
#include "Types/StructTypes.h"
#include "GASP_DF_MovementSettings.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_MovementSettings : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float InAirRotationYaw{200.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float SpeedMultiplier{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float BrakingFrictionFactor{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float BrakingFriction{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float BrakingDecelerationWalking{1500.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float GroundFriction{5.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float MinAnalogWalkSpeed{150.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float DefaultMaxWalkSpeed{500.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float MaxAcceleration{800.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float PerchRadiusThreshold{20.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	float CrouchedHalfHeight{60.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	bool bUseFlatBaseForFloorChecks{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	bool bUseSeparateBrakingFriction{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	bool bCanWalkOffLedgesWhenCrouching{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	bool bCanCrouch{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CharacterMovement")
	bool bUseAccelerationForPaths{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Networking")
	float MaxSmoothNetUpdateDist{92.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Networking")
	float NoSmoothNetUpdateDist{140.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	TMap<FGameplayTag, FGaitSettings> MovementSettings;
};
