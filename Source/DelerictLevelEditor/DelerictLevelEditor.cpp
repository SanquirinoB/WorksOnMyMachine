#include "ActorFactories/ActorFactoryBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "IPlacementModeModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "DelerictLevelEditor"

namespace
{
    const FName CategoryName(TEXT("DLevelDesign"));
    const FName PrefabPath(TEXT("/Game/Delerict/LevelDesign/Prefabs"));
}

class FDLevelEditorModule final : public IModuleInterface
{
    FDelegateHandle FilesLoadedHandle;

    void PopulatePalette()
    {
        IPlacementModeModule& Placement = IPlacementModeModule::Get();
        if (!Placement.RegisterPlacementCategory(FPlacementCategoryInfo(
            LOCTEXT("Category", "Delerict"), CategoryName, TEXT("DLevelDesign"), 25)))
        {
            return;
        }

        TArray<FAssetData> Assets;
        FAssetRegistryModule::GetRegistry().GetAssetsByPath(PrefabPath, Assets);
        UActorFactory* Factory = GEditor->FindActorFactoryByClass(UActorFactoryBlueprint::StaticClass());
        if (!Factory)
        {
            return;
        }
        for (const FAssetData& Asset : Assets)
        {
            FText Reason;
            if (Asset.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName()
                && Factory->CanCreateActorFrom(Asset, Reason))
            {
                Placement.RegisterPlaceableItem(CategoryName, MakeShared<FPlaceableItem>(Factory, Asset));
            }
        }
    }

public:
    void StartupModule() override
    {
        if (IsRunningCommandlet() || !GEditor)
        {
            return;
        }
        IAssetRegistry& Registry = FAssetRegistryModule::GetRegistry();
        if (Registry.IsLoadingAssets())
        {
            FilesLoadedHandle = Registry.OnFilesLoaded().AddRaw(this, &ThisClass::PopulatePalette);
        }
        else
        {
            PopulatePalette();
        }
    }

    void ShutdownModule() override
    {
        if (FilesLoadedHandle.IsValid())
        {
            if (FAssetRegistryModule* Registry = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
            {
                Registry->Get().OnFilesLoaded().Remove(FilesLoadedHandle);
            }
        }
        if (FModuleManager::Get().IsModuleLoaded("PlacementMode"))
        {
            IPlacementModeModule::Get().UnregisterPlacementCategory(CategoryName);
        }
    }

private:
    using ThisClass = FDLevelEditorModule;
};

IMPLEMENT_MODULE(FDLevelEditorModule, DelerictLevelEditor)
#undef LOCTEXT_NAMESPACE
