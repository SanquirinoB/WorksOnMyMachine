#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoreKitActionContext.generated.h"

/**
 * Optional request payload retained by an action while it executes.
 * Derive from this type to carry domain-specific data without coupling CoreKitActions
 * to inventory, interaction, locomotion, or another gameplay system.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class COREKITACTIONS_API UCoreKitActionContext : public UObject
{
	GENERATED_BODY()

public:
	/** Called before activation and again after cancellation callbacks mutate state. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "CoreKit Actions|Actions")
	bool IsActionContextValid() const;
};
