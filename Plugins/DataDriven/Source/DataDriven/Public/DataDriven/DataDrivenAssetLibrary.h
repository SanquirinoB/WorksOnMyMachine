#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DataDrivenAssetLibrary.generated.h"

/** Asset discovery and loading primitives used by DataDriven containers and subsystems. */
UCLASS()
class DATADRIVEN_API UDataDrivenAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Loads all assets whose class matches or derives from Class. */
	UFUNCTION(BlueprintCallable, Category = "DataDriven|Assets")
	static TArray<UObject*> LoadAssetsByClass(UClass* Class);

	/** Loads all registered primary assets under PrimaryAssetType. */
	UFUNCTION(BlueprintCallable, Category = "DataDriven|Assets")
	static TArray<UObject*> LoadPrimaryAssetsFromType(FPrimaryAssetType PrimaryAssetType);
};
