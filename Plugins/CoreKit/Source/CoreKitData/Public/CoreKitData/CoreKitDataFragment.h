#pragma once

#include "UObject/Object.h"
#include "CoreKitDataFragment.generated.h"

/** Base for authored values owned by a single data container. */
UCLASS(Abstract, BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew)
class COREKITDATA_API UCoreKitDataFragment : public UObject
{
	GENERATED_BODY()
};
