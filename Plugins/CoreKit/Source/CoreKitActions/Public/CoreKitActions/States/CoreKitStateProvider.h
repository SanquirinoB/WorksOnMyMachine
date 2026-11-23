#pragma once

#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "CoreKitStateProvider.generated.h"

UINTERFACE(BlueprintType)
class COREKITACTIONS_API UCoreKitStateProvider : public UInterface
{
	GENERATED_BODY()
};

/** Existing authoritative enum/state, exposed read-only. Implementations MUST have no side effects. */
class COREKITACTIONS_API ICoreKitStateProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="CoreKit Actions|States")
	FGameplayTag QueryDomainState(FName Domain) const;
	virtual FGameplayTag QueryDomainState_Implementation(FName Domain) const;
};
