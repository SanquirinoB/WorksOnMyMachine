#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataContainer.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "DataDrivenComponent.generated.h"

/** Component that maps authored Gameplay Tags to DataDriven containers. */
UCLASS(ClassGroup = (DataDriven), meta = (BlueprintSpawnableComponent))
class DATADRIVEN_API UDataDrivenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDataDrivenComponent();

	UFUNCTION(BlueprintPure, Category = "DataDriven")
	UDataContainer* GetDataContainer(UPARAM(meta = (Categories = "Container")) FGameplayTag InTag) const;

	UFUNCTION(BlueprintPure, Category = "DataDriven")
	bool HasDataContainer(UPARAM(meta = (Categories = "Container")) FGameplayTag InTag) const;

	/** Replaces the complete authored container map. Intended for reusable configuration sources. */
	void SetDataContainerMap(const TMap<FGameplayTag, TObjectPtr<UDataContainer>>& InDataContainerMap);

	/** Blueprint equivalent of SetDataContainerMap. */
	UFUNCTION(BlueprintCallable, Category = "DataDriven")
	void SetDataContainers(const TMap<FGameplayTag, UDataContainer*>& InDataContainers);

	/** Adds or replaces one container. Invalid tags and null containers are rejected. */
	UFUNCTION(BlueprintCallable, Category = "DataDriven")
	bool SetDataContainer(UPARAM(meta = (Categories = "Container")) FGameplayTag InTag, UDataContainer* InDataContainer);

	UFUNCTION(BlueprintCallable, Category = "DataDriven")
	bool RemoveDataContainer(UPARAM(meta = (Categories = "Container")) FGameplayTag InTag);

	UFUNCTION(BlueprintCallable, Category = "DataDriven")
	void ClearDataContainerMap();

	const TMap<FGameplayTag, TObjectPtr<UDataContainer>>& GetDataContainerMap() const { return DataContainerMap; }

	UFUNCTION(BlueprintPure, Category = "DataDriven", DisplayName = "Get Data Containers")
	TMap<FGameplayTag, UDataContainer*> BP_GetDataContainers() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataDriven")
	TMap<FGameplayTag, TObjectPtr<UDataContainer>> DataContainerMap;
};
