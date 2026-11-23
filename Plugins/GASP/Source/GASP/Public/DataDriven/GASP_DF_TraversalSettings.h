#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "Engine/StreamableManager.h"
#include "GASP_DF_TraversalSettings.generated.h"

UCLASS(BlueprintType, EditInlineNew)
class GASP_API UGASP_DF_TraversalSettings : public UDataFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (ClampMin = "0.0"))
	float IgnoreCorrectionDelay{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (ClampMin = "0.0"))
	float MinLedgeWidth{30.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (ClampMin = "0.0"))
	float MinFrontLedgeDepth{37.522631f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	TSoftObjectPtr<class UChooserTable> TraversalAnimationsChooserTable;
};
