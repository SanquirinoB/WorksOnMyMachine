#pragma once

#include "CoreMinimal.h"
#include "DataDrivenTypes.generated.h"

/** Result of a class- or tag-based DataDriven lookup. */
UENUM(BlueprintType)
enum class EDataDrivenLookupResult : uint8
{
	Success,
	InvalidContainer,
	InvalidFragmentClass,
	FragmentNotFound,
	InvalidTag,
	ContainerNotFound
};
