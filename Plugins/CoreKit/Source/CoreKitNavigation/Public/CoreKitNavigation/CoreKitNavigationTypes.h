#pragma once

#include "CoreMinimal.h"
#include "CoreKitNavigationTypes.generated.h"

UENUM(BlueprintType)
enum class ECoreKitInputDevice : uint8
{
	Keyboard,
	Mouse,
	Gamepad
};
