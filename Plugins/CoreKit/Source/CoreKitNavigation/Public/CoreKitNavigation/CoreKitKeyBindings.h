#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "CoreKitKeyBindings.generated.h"

class UInputMappingContext;
class ULocalPlayer;

UENUM(BlueprintType)
enum class ECoreKitBindingDevice : uint8
{
	Any,
	KeyboardAndMouse,
	Gamepad
};

UENUM(BlueprintType)
enum class ECoreKitBindingResult : uint8
{
	Success,
	InvalidBinding,
	WrongDevice,
	Conflict,
	SettingsRejected
};

USTRUCT(BlueprintType)
struct COREKITNAVIGATION_API FCoreKitBindingReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation")
	ECoreKitBindingResult Result = ECoreKitBindingResult::InvalidBinding;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation")
	TArray<FName> ConflictingMappings;
};

UCLASS()
class COREKITNAVIGATION_API UCoreKitKeyBindings : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Supply contexts that can be active with the target. Retry with Swap to exchange one conflicting key. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	static FCoreKitBindingReport ApplyKeyBinding(ULocalPlayer* Player, UInputMappingContext* Context,
		const FMapPlayerKeyArgs& Binding, const TArray<UInputMappingContext*>& ConcurrentContexts,
		ECoreKitBindingDevice Device = ECoreKitBindingDevice::Any, bool bSwap = false);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	static ECoreKitBindingResult RestoreBindingDefaults(ULocalPlayer* Player, const FMapPlayerKeyArgs& Binding);
};
