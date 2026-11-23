#include "CoreKitData/CoreKitDataAssetLibrary.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CoreKitData/CoreKitDataContainer.h"

namespace
{
TArray<FAssetData> FindContainerAssets(const UClass* ContainerClass, FName PackagePath)
{
	if (!ContainerClass)
	{
		return {};
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
#if WITH_EDITOR
	// An uncooked standalone game can query before the editor starts its initial scan.
	if (!AssetRegistry.IsSearchAllAssets())
	{
		if (PackagePath.IsNone())
		{
			AssetRegistry.SearchAllAssets(true);
		}
		else
		{
			AssetRegistry.ScanPathsSynchronous({PackagePath.ToString()});
		}
	}
#endif
	AssetRegistry.WaitForCompletion();

	FARFilter Filter;
	Filter.ClassPaths.Add(ContainerClass->GetClassPathName());
	Filter.bRecursiveClasses = true;
	if (!PackagePath.IsNone())
	{
		Filter.PackagePaths.Add(PackagePath);
		Filter.bRecursivePaths = true;
	}

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.GetSoftObjectPath().LexicalLess(Right.GetSoftObjectPath());
	});
	return Assets;
}
}

TArray<TSoftObjectPtr<UCoreKitDataContainer>> UCoreKitDataAssetLibrary::DiscoverDataContainers(
	TSubclassOf<UCoreKitDataContainer> ContainerClass, FName PackagePath)
{
	const auto Assets = FindContainerAssets(ContainerClass.Get(), PackagePath);
	TArray<TSoftObjectPtr<UCoreKitDataContainer>> References;
	References.Reserve(Assets.Num());
	for (const FAssetData& Asset : Assets)
	{
		References.Emplace(Asset.GetSoftObjectPath());
	}
	return References;
}

TArray<UCoreKitDataContainer*> UCoreKitDataAssetLibrary::LoadDataContainers(
	TSubclassOf<UCoreKitDataContainer> ContainerClass, FName PackagePath)
{
	const UClass* RequestedClass = ContainerClass.Get();
	const auto Assets = FindContainerAssets(RequestedClass, PackagePath);
	TArray<UCoreKitDataContainer*> Containers;
	Containers.Reserve(Assets.Num());
	for (const FAssetData& Asset : Assets)
	{
		UCoreKitDataContainer* Container = Cast<UCoreKitDataContainer>(Asset.GetAsset());
		if (IsValid(Container) && Container->IsA(RequestedClass))
		{
			Containers.Add(Container);
		}
	}

	return Containers;
}
