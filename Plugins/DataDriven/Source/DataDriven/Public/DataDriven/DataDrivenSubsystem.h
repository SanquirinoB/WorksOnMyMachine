#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataDrivenAssetLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataDrivenSubsystem.generated.h"

/** Base GameInstance subsystem for projects that discover DataDriven assets at runtime. */
UCLASS(Abstract)
class DATADRIVEN_API UDataDrivenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	/** Loads all assets compatible with T and retains them for the subsystem lifetime. */
	template <class T>
	TArray<T*> LoadContainersFromAssets();

	UPROPERTY()
	TArray<UObject*> RawDataAssets;
};

template <class T>
TArray<T*> UDataDrivenSubsystem::LoadContainersFromAssets()
{
	RawDataAssets.Reset();
	RawDataAssets = UDataDrivenAssetLibrary::LoadAssetsByClass(T::StaticClass());

	TArray<T*> CurrentArrayAsset;
	CurrentArrayAsset.Reserve(RawDataAssets.Num());

	for (UObject* DataAsset : RawDataAssets)
	{
		if (IsValid(DataAsset) && DataAsset->IsA(T::StaticClass()))
		{
			CurrentArrayAsset.Add(CastChecked<T>(DataAsset));
		}
	}

	return CurrentArrayAsset;
}
