#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "GASP_DF_AnimationBindingSettings.generated.h"

class UChooserTable;

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_AnimationBindingSettings : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationBinding")
	TObjectPtr<class UChooserTable> OverlayTable{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationBinding")
	TObjectPtr<UChooserTable> PosesTable{nullptr};
};
