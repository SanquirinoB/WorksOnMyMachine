#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataDrivenTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DataDrivenBlueprintLibrary.generated.h"

class UDataContainer;
class UDataDrivenComponent;
class UDataFragmentBase;

/** Blueprint-facing class/tag lookup helpers for DataDriven assets. */
UCLASS()
class DATADRIVEN_API UDataDrivenBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DataDriven|Fragments", meta = (DeterminesOutputType = "FragmentClass", ExpandEnumAsExecs = "Result"))
	static UDataFragmentBase* GetDataFragmentByClass(
		UDataContainer* InDataContainer,
		TSubclassOf<UDataFragmentBase> FragmentClass,
		EDataDrivenLookupResult& Result);

	UFUNCTION(BlueprintCallable, Category = "DataDriven|Fragments", meta = (DeterminesOutputType = "FragmentClass", ExpandEnumAsExecs = "Result"))
	static TArray<UDataFragmentBase*> GetDataFragmentsByClass(
		UDataContainer* InDataContainer,
		TSubclassOf<UDataFragmentBase> FragmentClass,
		EDataDrivenLookupResult& Result);

	UFUNCTION(BlueprintPure, Category = "DataDriven|Fragments")
	static bool HasDataFragmentOfClass(UDataContainer* InDataContainer, TSubclassOf<UDataFragmentBase> FragmentClass);

	UFUNCTION(BlueprintCallable, Category = "DataDriven|Containers", meta = (ExpandEnumAsExecs = "Result"))
	static UDataContainer* GetDataContainerByTag(
		UDataDrivenComponent* InComponent,
		FGameplayTag ContainerTag,
		EDataDrivenLookupResult& Result);
};
