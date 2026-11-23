#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "IPlacementModeModule.h"
#include "GameFramework/DefaultPawn.h"
#include "LevelDesign/DDockingPoint.h"
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
    TSharedPtr<FPlaceableItem> DockItem;
    for (const TSharedPtr<FPlaceableItem>& Item : Items)
    {
        if (Item && Item->AssetFactory.GetInterface())
        {
            Names.Add(Item->AssetData.AssetName);
            if (Item->AssetData.AssetName == TEXT("BP_DPlatform"))
            {
                PlatformItem = Item;
            }
            if (Item->AssetData.AssetName == TEXT("BP_DDockingPoint"))
            {
                DockItem = Item;
            }
        }
    }
    for (const TCHAR* Prefab : {TEXT("BP_DFloor"), TEXT("BP_DPlatform"), TEXT("BP_DWall"),
        TEXT("BP_DPlayerStart"), TEXT("BP_DHazard"), TEXT("BP_DCheckpoint"), TEXT("BP_DExit")})
    {
        TestTrue(FString::Printf(TEXT("Place Actors offers %s with an actor factory"), Prefab), Names.Contains(Prefab));
    }
    if (!PlatformItem || !PlatformItem->Factory || !TestTrue(TEXT("The docking prefab has a placement factory"),
        DockItem.IsValid() && DockItem->Factory != nullptr))
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
    const FTransform Mount(FRotator(90., 0., 0.), FVector(1500., 0., 400.));
    auto* Dock = Cast<ADDockingPoint>(DockItem->Factory->CreateActor(
        DockItem->AssetData.GetAsset(), World->PersistentLevel, Mount));
    if (!TestNotNull(TEXT("The factory places the docking Blueprint"), Dock))
    {
        return false;
    }
    const FVector Position = Mount.TransformPositionNoScale(Dock->DockingOffset);
    APawn* Probe = World->SpawnActor<ADefaultPawn>(Position, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Probe pawn"), Probe))
    {
        return false;
    }
    FTransform Pose;
    TestTrue(TEXT("The nearby pawn receives the oriented docking pose"), Dock->TryGetDockingPose(Probe, Pose)
        && Pose.GetLocation().Equals(Position) && Pose.GetRotation().Equals(Mount.GetRotation()));
    Dock->bDockingEnabled = false;
    TestFalse(TEXT("A disabled dock rejects the nearby pawn"), Dock->TryGetDockingPose(Probe, Pose));
    Dock->bDockingEnabled = true;
    Probe->SetActorLocation(Position + FVector(Dock->ApproachRadius + 50., 0., 0.));
    TestFalse(TEXT("An enabled dock rejects a distant pawn"), Dock->TryGetDockingPose(Probe, Pose));
    return true;
}

#endif
