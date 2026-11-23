#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "Types/EnumTypes.h"
#include "GASP_DF_LocomotionSettings.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_LocomotionSettings : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogMovementThreshold{0.7f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	EAnalogStickBehaviorMode MovementStickMode{EAnalogStickBehaviorMode::FixedSingleGait};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SprintMaxDirectionAngle{50.0f};
};


