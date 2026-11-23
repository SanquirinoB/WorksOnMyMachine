#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoreKitActionSource.generated.h"

class UCoreKitActionSet;

UINTERFACE(BlueprintType)
class COREKITACTIONS_API UCoreKitActionSource : public UInterface
{
	GENERATED_BODY()
};

/** Optional provider used when an Action Set is not assigned directly on the component. */
class COREKITACTIONS_API ICoreKitActionSource
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CoreKit Actions|Actions")
	UCoreKitActionSet* GetAFActionSet() const;
};
