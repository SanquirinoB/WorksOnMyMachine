#include "DataDriven/DataDrivenAssetLibrary.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

TArray<UObject*> UDataDrivenAssetLibrary::LoadAssetsByClass(UClass* Class)
{
	TArray<UObject*> LoadedAssets;
	if (!IsValid(Class))
	{
		return LoadedAssets;
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassPaths.Add(Class->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
	AssetDataList.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.ToString() < Right.PackageName.ToString();
	});

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (UObject* Asset = AssetData.GetAsset())
		{
			if (Asset->IsA(Class))
			{
				LoadedAssets.Add(Asset);
			}
		}
	}

	return LoadedAssets;
}

TArray<UObject*> UDataDrivenAssetLibrary::LoadPrimaryAssetsFromType(const FPrimaryAssetType PrimaryAssetType)
{
	TArray<UObject*> LoadedAssets;
	if (!PrimaryAssetType.IsValid())
	{
		return LoadedAssets;
	}

	if (UAssetManager* Manager = UAssetManager::GetIfInitialized())
	{
		if (const TSharedPtr<FStreamableHandle> Handle = Manager->LoadPrimaryAssetsWithType(PrimaryAssetType); Handle.IsValid())
		{
			Handle->WaitUntilComplete(0.0f, false);
			Handle->GetLoadedAssets(LoadedAssets);
		}
	}

	return LoadedAssets;
}
