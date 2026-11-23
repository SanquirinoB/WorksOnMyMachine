#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CoreKitDataComponent.generated.h"

class UCoreKitDataContainer;

/** Actor-local registry of authored data containers keyed by exact Gameplay Tags. */
UCLASS(ClassGroup = (CoreKit), meta = (BlueprintSpawnableComponent))
class COREKITDATA_API UCoreKitDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Returns the container registered for the exact tag, or null if the key or value is missing. */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Data")
	UCoreKitDataContainer* FindDataContainer(FGameplayTag Tag) const;

protected:
	UPROPERTY(EditAnywhere, Category = "CoreKit|Data")
	TMap<FGameplayTag, TObjectPtr<UCoreKitDataContainer>> Containers;
};
