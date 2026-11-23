#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DataFragmentBase.generated.h"

/** Base class for designer-authored, instanced data fragments. */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, BlueprintType, Blueprintable, ClassGroup = (DataDriven))
class DATADRIVEN_API UDataFragmentBase : public UObject
{
	GENERATED_BODY()
};
