#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIArenaMinimapDefinition.h"

#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalTypes.h"

FVector2D UCoreKitBasicUIArenaMinimapDefinition::WorldToUV(FVector2D WorldXY) const
{
	return CoreKitBasicUIWorldSignals::WorldToPlanUV(WorldXY, BoundsMin, BoundsMax);
}

UCoreKitBasicUIArenaMinimapDefinition* UCoreKitBasicUIArenaMinimapDefinition::FindForMapName(const FString& MapShortName)
{
	if (MapShortName.IsEmpty())
	{
		return nullptr;
	}
	const FString Name = FString::Printf(TEXT("DA_Minimap_%s"), *MapShortName);
	const FString Path = FString::Printf(TEXT("/CoreKit/BasicUI/Minimap/%s.%s"), *Name, *Name);
	return LoadObject<UCoreKitBasicUIArenaMinimapDefinition>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UCoreKitBasicUIArenaMinimapDefinition* UCoreKitBasicUIArenaMinimapDefinition::FindForWorld(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	// PIE prefixes the package name (UEDPIE_0_L_CoreKitBasicUIArena); strip it to find the plan of the source map.
	FString Short = FPackageName::GetShortName(World->GetOutermost()->GetName());
	Short = UWorld::RemovePIEPrefix(Short);
	return FindForMapName(Short);
}
