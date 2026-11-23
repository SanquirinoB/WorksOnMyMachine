#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "GASP_DF_RagdollSettings.generated.h"

class UAnimMontage;

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_RagdollSettings : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	TObjectPtr<class UAnimMontage> GetUpMontageFront{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	TObjectPtr<UAnimMontage> GetUpMontageBack{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll")
	bool bLimitInitialRagdollSpeed{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float MontageBlendOutDuration{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float MinInitialSpeedLimit{200.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float PullForce{750.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float PullForceInterpolationSpeed{0.6f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float PullForceMinDistance{5.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float PullForceMaxDistance{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float SpinePullSpeedThreshold{300.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float MotorReferenceSpeed{1000.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float MotorStiffness{25000.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdoll", meta = (ClampMin = "0.0"))
	float FlailPlayRateReferenceSpeed{1000.0f};
};
