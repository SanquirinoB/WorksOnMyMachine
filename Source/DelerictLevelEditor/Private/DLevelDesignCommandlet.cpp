#include "DLevelDesignCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "LevelDesign/DLevelBlock.h"
#include "LevelDesign/DRoomMarker.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogDLevelDesign, Log, All);

namespace
{
    const FString KitPath(TEXT("/Game/Delerict/LevelDesign"));
    const FString LabPath(TEXT("/Game/Maps/L_DLevelDesignLab"));

    bool SaveAsset(UObject& Asset)
    {
        UPackage* Package = Asset.GetPackage();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FAssetRegistryModule::AssetCreated(&Asset);
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, &Asset, *Filename, Args);
    }

    UMaterialInstanceConstant* FindOrCreateFinish(const TCHAR* Name, const FLinearColor& Tint)
    {
        const FString Path = KitPath / TEXT("Finishes") / Name;
        if (FPackageName::DoesPackageExist(Path))
        {
            return LoadObject<UMaterialInstanceConstant>(nullptr, *(Path + TEXT(".") + Name));
        }
        UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (!Parent)
        {
            return nullptr;
        }
        auto* Finish = NewObject<UMaterialInstanceConstant>(CreatePackage(*Path), FName(Name), RF_Public | RF_Standalone);
        Finish->SetParentEditorOnly(Parent);
        Finish->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Color")), Tint);
        Finish->PostEditChange();
        return SaveAsset(*Finish) ? Finish : nullptr;
    }

    UClass* FindOrCreatePrefab(const TCHAR* Name, UClass* Parent, TFunctionRef<void(AActor&)> Configure)
    {
        const FString Path = KitPath / TEXT("Prefabs") / Name;
        if (FPackageName::DoesPackageExist(Path))
        {
            UBlueprint* Existing = LoadObject<UBlueprint>(nullptr, *(Path + TEXT(".") + Name));
            return Existing && Existing->GeneratedClass && Existing->GeneratedClass->IsChildOf(Parent)
                ? Existing->GeneratedClass.Get() : nullptr;
        }
        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, CreatePackage(*Path), FName(Name),
            BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("DLevelDesign"));
        if (!Blueprint || !Blueprint->GeneratedClass)
        {
            return nullptr;
        }
        Configure(*Blueprint->GeneratedClass->GetDefaultObject<AActor>());
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        return Blueprint->Status != BS_Error && SaveAsset(*Blueprint) ? Blueprint->GeneratedClass.Get() : nullptr;
    }

    UClass* BuildBlock(const TCHAR* Name, const FVector& Dimensions, UMaterialInterface* Finish)
    {
        return FindOrCreatePrefab(Name, ADLevelBlock::StaticClass(), [&](AActor& Actor)
        {
            auto& Block = *CastChecked<ADLevelBlock>(&Actor);
            Block.Dimensions = Dimensions;
            Block.Finish = Finish;
        });
    }

    UClass* BuildMarker(const TCHAR* Name, EDRoomMarkerRole Role, const FVector& Dimensions,
        UMaterialInterface* Finish, const TCHAR* Label, const TCHAR* Feedback = TEXT(""))
    {
        return FindOrCreatePrefab(Name, ADRoomMarker::StaticClass(), [&](AActor& Actor)
        {
            auto& Marker = *CastChecked<ADRoomMarker>(&Actor);
            Marker.MarkerRole = Role;
            Marker.Dimensions = Dimensions;
            Marker.Finish = Finish;
            Marker.Label = FText::FromString(Label);
            Marker.Feedback = FText::FromString(Feedback);
        });
    }

    AActor* Place(UWorld& World, UClass* Class, const TCHAR* Name, const FVector& Location,
        const FRotator& Rotation = FRotator::ZeroRotator)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* Actor = World.SpawnActor<AActor>(Class, Location, Rotation, Params);
        if (Actor)
        {
            Actor->SetActorLabel(Name);
        }
        return Actor;
    }

    template <typename T>
    bool PlaceSized(UWorld& World, UClass* Class, const TCHAR* Name, const FVector& Location, const FVector& Size)
    {
        const FTransform Transform(Location);
        T* Actor = World.SpawnActorDeferred<T>(Class, Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Actor)
        {
            return false;
        }
        Actor->Dimensions = Size;
        Actor->FinishSpawning(Transform);
        Actor->SetActorLabel(Name);
        return true;
    }
}

UDLevelDesignCommandlet::UDLevelDesignCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UDLevelDesignCommandlet::Main(const FString&)
{
    auto* Steel = FindOrCreateFinish(TEXT("MI_DSteel"), FLinearColor(.13f, .25f, .39f));
    auto* Step = FindOrCreateFinish(TEXT("MI_DPlatform"), FLinearColor(.18f, .58f, .66f));
    auto* Red = FindOrCreateFinish(TEXT("MI_DHazard"), FLinearColor(.92f, .08f, .037f));
    auto* Green = FindOrCreateFinish(TEXT("MI_DCheckpoint"), FLinearColor(.08f, .9f, .45f));
    auto* Blue = FindOrCreateFinish(TEXT("MI_DExit"), FLinearColor(.2f, .53f, 1.f));
    if (!Steel || !Step || !Red || !Green || !Blue)
    {
        UE_LOG(LogDLevelDesign, Error, TEXT("Unable to prepare the kit materials"));
        return 1;
    }

    UClass* Floor = BuildBlock(TEXT("BP_DFloor"), FVector(600, 300, 50), Steel);
    UClass* Platform = BuildBlock(TEXT("BP_DPlatform"), FVector(400, 300, 40), Step);
    UClass* Wall = BuildBlock(TEXT("BP_DWall"), FVector(50, 300, 260), Steel);
    UClass* Start = FindOrCreatePrefab(TEXT("BP_DPlayerStart"), APlayerStart::StaticClass(), [](AActor&) {});
    UClass* Hazard = BuildMarker(TEXT("BP_DHazard"), EDRoomMarkerRole::Hazard, FVector(100, 300, 35),
        Red, TEXT("PELIGRO"));
    UClass* Checkpoint = BuildMarker(TEXT("BP_DCheckpoint"), EDRoomMarkerRole::Checkpoint,
        FVector(110, 300, 150), Green, TEXT("CHECKPOINT"), TEXT("Checkpoint activado"));
    UClass* Exit = BuildMarker(TEXT("BP_DExit"), EDRoomMarkerRole::Exit, FVector(110, 300, 180),
        Blue, TEXT("SALIDA"), TEXT("Sala completada"));
    if (!Floor || !Platform || !Wall || !Start || !Hazard || !Checkpoint || !Exit)
    {
        UE_LOG(LogDLevelDesign, Error, TEXT("Unable to prepare all seven kit prefabs"));
        return 1;
    }
    if (FPackageName::DoesPackageExist(LabPath))
    {
        UE_LOG(LogDLevelDesign, Display, TEXT("Existing lab preserved: %s"), *LabPath);
        return 0;
    }

    UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
    if (!World)
    {
        UE_LOG(LogDLevelDesign, Error, TEXT("Could not create the lab world"));
        return 1;
    }
    const bool bPlaced = Place(*World, Start, TEXT("01 Player Start"), FVector(0, 0, 100))
        && PlaceSized<ADLevelBlock>(*World, Floor, TEXT("02 Approach"), FVector(175, 0, -50), FVector(950, 300, 50))
        && Place(*World, Platform, TEXT("03 Landing"), FVector(1000, 0, 20))
        && PlaceSized<ADLevelBlock>(*World, Floor, TEXT("04 Corridor"), FVector(1900, 0, -50), FVector(1400, 300, 50))
        && PlaceSized<ADRoomMarker>(*World, Hazard, TEXT("05 Gap recovery"), FVector(725, 0, -300), FVector(150, 300, 100))
        && Place(*World, Checkpoint, TEXT("06 Checkpoint"), FVector(1450, 0, 0))
        && Place(*World, Hazard, TEXT("07 Jump over hazard"), FVector(1775, 0, 0))
        && Place(*World, Exit, TEXT("08 Exit"), FVector(2250, 0, 0))
        && Place(*World, Wall, TEXT("09 End Wall"), FVector(2550, 0, 0));

    auto* KeyLight = Cast<ADirectionalLight>(Place(*World, ADirectionalLight::StaticClass(),
        TEXT("Lab Key Light"), FVector(0, 0, 400), FRotator(-42, -55, 0)));
    auto* Fill = Cast<ADirectionalLight>(Place(*World, ADirectionalLight::StaticClass(),
        TEXT("Lab Fill"), FVector(0, 0, 350), FRotator(-25, -137, 0)));
    if (!bPlaced || !KeyLight || !Fill)
    {
        UE_LOG(LogDLevelDesign, Error, TEXT("Could not place the lab actors"));
        return 1;
    }
    KeyLight->GetLightComponent()->SetIntensity(3.f);
    Fill->GetLightComponent()->SetIntensity(1.2f);
    Fill->GetLightComponent()->SetCastShadows(false);
    if (!UEditorLoadingAndSavingUtils::SaveMap(World, LabPath))
    {
        UE_LOG(LogDLevelDesign, Error, TEXT("Unable to save the pilot map"));
        return 1;
    }
    UE_LOG(LogDLevelDesign, Display, TEXT("Created %s and its seven Blueprint prefabs"), *LabPath);
    return 0;
}
