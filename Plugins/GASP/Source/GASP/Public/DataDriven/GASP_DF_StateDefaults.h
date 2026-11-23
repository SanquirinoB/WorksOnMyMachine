#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "GameplayTagContainer.h"
#include "GASP_DF_StateDefaults.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_StateDefaults : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag DesiredGait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag RotationMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag MovementState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag MovementMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag StanceMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag OverlayMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag PoseMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FGameplayTag LocomotionAction;
};


