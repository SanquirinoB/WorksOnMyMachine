#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "IPlacementModeModule.h"
#include "LevelDesign/DLevelBlock.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLevelPaletteTest, "Delerict.LevelDesign.Palette",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDLevelPaletteTest::RunTest(const FString&)
{
    FAssetRegistryModule::GetRegistry().WaitForCompletion();
    TArray<TSharedPtr<FPlaceableItem>> Items;
    IPlacementModeModule::Get().GetItemsForCategory(TEXT("DLevelDesign"), Items);
    TSet<FName> Names;
    TSharedPtr<FPlaceableItem> PlatformItem;
    for (const TSharedPtr<FPlaceableItem>& Item : Items)
    {
        if (Item && Item->AssetFactory.GetInterface())
        {
            Names.Add(Item->AssetData.AssetName);
            if (Item->AssetData.AssetName == TEXT("BP_DPlatform"))
            {
                PlatformItem = Item;
            }
        }
    }
    for (const TCHAR* Prefab : {TEXT("BP_DFloor"), TEXT("BP_DPlatform"), TEXT("BP_DWall"),
        TEXT("BP_DPlayerStart"), TEXT("BP_DHazard"), TEXT("BP_DCheckpoint"), TEXT("BP_DExit")})
    {
        TestTrue(FString::Printf(TEXT("Place Actors offers %s with an actor factory"), Prefab), Names.Contains(Prefab));
    }
    if (!PlatformItem || !PlatformItem->Factory)
    {
        return false;
    }
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    if (!TestNotNull(TEXT("Test map"), World))
    {
        return false;
    }
    auto* Platform = Cast<ADLevelBlock>(PlatformItem->Factory->CreateActor(
        PlatformItem->AssetData.GetAsset(), World->PersistentLevel, FTransform(FVector(500, 0, 100))));
    if (!TestNotNull(TEXT("The palette factory places an editable Blueprint platform"), Platform))
    {
        return false;
    }

    const FVector Size(650, 200, 70);
    Platform->Dimensions = Size;
    Platform->PostEditChange();
    const auto* Collision = Platform->FindComponentByClass<UBoxComponent>();
    const auto* Visual = Platform->FindComponentByClass<UStaticMeshComponent>();
    TestTrue(TEXT("Editing Dimensions updates the collision and mesh around the bottom pivot"),
        Collision && Visual && Collision->GetScaledBoxExtent().Equals(Size * .5)
        && Visual->Bounds.BoxExtent.Equals(Size * .5, .5)
        && FMath::IsNearlyEqual(Collision->GetComponentLocation().Z, Platform->GetActorLocation().Z + Size.Z * .5));
    return true;
}

#endif
