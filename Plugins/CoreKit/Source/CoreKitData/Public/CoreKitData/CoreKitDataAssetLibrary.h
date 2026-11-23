#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "CoreKitDataAssetLibrary.generated.h"

class UCoreKitDataContainer;

UCLASS()
class COREKITDATA_API UCoreKitDataAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Queries the Asset Registry without loading the matching objects. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Data")
	static TArray<TSoftObjectPtr<UCoreKitDataContainer>> DiscoverDataContainers(
		TSubclassOf<UCoreKitDataContainer> ContainerClass, FName PackagePath = NAME_None);

	/** Synchronously loads matching containers in object path order. Pass None to search all mounted content. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Data", meta = (DeterminesOutputType = "ContainerClass"))
	static TArray<UCoreKitDataContainer*> LoadDataContainers(
		TSubclassOf<UCoreKitDataContainer> ContainerClass,
		FName PackagePath = NAME_None);
};
