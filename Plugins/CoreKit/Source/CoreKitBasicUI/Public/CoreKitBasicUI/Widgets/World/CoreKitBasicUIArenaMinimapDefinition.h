#pragma once

#include "Engine/DataAsset.h"
#include "CoreKitBasicUIArenaMinimapDefinition.generated.h"

class UTexture2D;
class UWorld;

/**
 * Minimap plan of one arena, produced by Tools/UI/CaptureArenaPlan.py (top-down orthographic SceneCapture, stylised:
 * dark plate, obstacles in Shade/Seam). The texture covers BoundsMin..BoundsMax in world XY.
 *
 * Orientation (north up): image up = world +X, image right = world +Y. Use WorldToUV for any world point.
 * Assets: /CoreKit/BasicUI/Minimap/DA_Minimap_<MapName> next to T_Minimap_<Arena>.
 */
UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIArenaMinimapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Arena name shown under the minimap ("ROOFTOP", HUD mockup). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") FText ArenaName;
	/** Map this plan was captured from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") TSoftObjectPtr<UWorld> Map;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") TSoftObjectPtr<UTexture2D> PlanTexture;
	/** World XY covered by the texture (min = bottom-left corner in world X/Y, see the class comment). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") FVector2D BoundsMin = FVector2D(-2500.f, -2500.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") FVector2D BoundsMax = FVector2D(2500.f, 2500.f);
	/** World XY of the playable floor inside the bounds (for clamping icons); equals the bounds when unknown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") FVector2D PlayableMin = FVector2D(-2500.f, -2500.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap") FVector2D PlayableMax = FVector2D(2500.f, 2500.f);
	/** Capture metadata (orthographic width in cm, texture size in px, capture date) for re-rendering. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture") float CaptureOrthoWidth = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture") int32 CaptureResolution = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture") FString CaptureNotes;

	/** World XY -> texture UV (0..1), north up. */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Minimap") FVector2D WorldToUV(FVector2D WorldXY) const;

	/** Loads /CoreKit/BasicUI/Minimap/DA_Minimap_<MapShortName> for a world (nullptr when the map has no plan). */
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI|Minimap", meta = (WorldContext = "WorldContext"))
	static UCoreKitBasicUIArenaMinimapDefinition* FindForWorld(const UObject* WorldContext);
	static UCoreKitBasicUIArenaMinimapDefinition* FindForMapName(const FString& MapShortName);
};
