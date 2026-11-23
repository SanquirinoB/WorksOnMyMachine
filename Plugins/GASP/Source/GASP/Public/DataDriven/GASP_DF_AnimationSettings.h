#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "BoneControllers/AnimNode_FootPlacement.h"
#include "GameplayTagContainer.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Types/StructTypes.h"
#include "GASP_DF_AnimationSettings.generated.h"

class UAnimSequenceBase;
class UChooserTable;

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_AnimationSettings : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Choosers")
	TObjectPtr<class UChooserTable> LocomotionTable{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Choosers")
	TObjectPtr<UChooserTable> StateMachineTable{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Trajectory")
	FPoseSearchTrajectoryData TrajectoryGenerationData_Idle{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Trajectory")
	FPoseSearchTrajectoryData TrajectoryGenerationData_Moving{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|FootPlacement")
	FFootPlacementPlantSettings PlantSettings_Default{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|FootPlacement")
	FFootPlacementPlantSettings PlantSettings_Stops{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|FootPlacement")
	FFootPlacementInterpolationSettings InterpolationSettings_Default{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|FootPlacement")
	FFootPlacementInterpolationSettings InterpolationSettings_Stops{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|StateMachine")
	float HeavyLandSpeedThreshold{700.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|StateMachine")
	TMap<FGameplayTag, FPivotSettings> PivotSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|StateMachine")
	TObjectPtr<UAnimSequenceBase> StrafeCurveAnimationAsset{nullptr};
};
