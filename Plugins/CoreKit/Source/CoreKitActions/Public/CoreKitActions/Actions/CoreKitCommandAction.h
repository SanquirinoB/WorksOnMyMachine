#pragma once

#include "CoreMinimal.h"
#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitCommandAction.generated.h"

/**
 * Policy marker for synchronous, payload-bearing commands. The coordinator executes a
 * native, typed callback under its mutation guard; it NEVER instantiates this task.
 * A dispatched command is not a finished montage/session. Domain systems own that state.
 */
UCLASS(NotBlueprintable)
class COREKITACTIONS_API UCoreKitCommandAction : public UCoreKitAction
{
	GENERATED_BODY()

public:
	virtual bool EvaluateActivation_Implementation(FName& FailureCode, FString& Detail) const override;
	static bool ValidatePolicy(const UCoreKitActionDefinition* Definition, FString& Detail);
};
