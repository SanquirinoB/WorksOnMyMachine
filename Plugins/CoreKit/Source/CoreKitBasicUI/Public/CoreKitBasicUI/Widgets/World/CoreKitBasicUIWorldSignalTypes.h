#pragma once

#include "Engine/DeveloperSettings.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUIWorldSignalTypes.generated.h"

class UMaterialInterface;

/**
 * World signals (stage UI-W, Docs/UI/Spec/03-HUDEvents.md "Señales de mundo"): editable references and timings.
 * Project Settings > Game > CoreKitBasicUI World Signals.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CoreKitBasicUI World Signals"))
class COREKITBASICUI_API UCoreKitBasicUIWorldSignalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIWorldSignalSettings();
	FName GetCategoryName() const override { return TEXT("Game"); }

	/** Floor ring for the Heavy Mech Ground Slam (orange solid + dashed inner ring, radius = real slam radius). */
	UPROPERTY(Config, EditAnywhere, Category = "Materials") TSoftObjectPtr<UMaterialInterface> SlamRingMaterial;
	/** Emissive mint material for repair kits (health pickups) that still use an engine basic shape (legacy drops). */
	UPROPERTY(Config, EditAnywhere, Category = "Materials") TSoftObjectPtr<UMaterialInterface> RepairKitMaterial;
	/** Mint fresnel rim drawn as an overlay on repair kits with their own art (SM_MedicalKit keeps its textures). */
	UPROPERTY(Config, EditAnywhere, Category = "Materials") TSoftObjectPtr<UMaterialInterface> RepairKitRimMaterial;

	/** Seconds the damaged-enemy bar stays after the last hit before it fades. */
	UPROPERTY(Config, EditAnywhere, Category = "Health bar", meta = (Units = "s")) float HealthBarLinger = 5.f;
	/** Height above the capsule top where the bar sits (cm). */
	UPROPERTY(Config, EditAnywhere, Category = "Health bar", meta = (Units = "cm")) float HealthBarHeight = 34.f;

	/** World radius used to size the CHARGING ring around a drone (cm); the projected ring is clamped to Min/Max px. */
	UPROPERTY(Config, EditAnywhere, Category = "Charge ring", meta = (Units = "cm")) float ChargeRingWorldRadius = 140.f;
	UPROPERTY(Config, EditAnywhere, Category = "Charge ring") float ChargeRingMinPx = 120.f;
	UPROPERTY(Config, EditAnywhere, Category = "Charge ring") float ChargeRingMaxPx = 280.f;


	/** Floating-text style ids (rows of DT_CoreKitBasicUIDamageNumberStyles). */
	UPROPERTY(Config, EditAnywhere, Category = "Damage numbers") FName RifleStyle = TEXT("CoreKitBasicUIRifle");
	UPROPERTY(Config, EditAnywhere, Category = "Damage numbers") FName KatanaStyle = TEXT("CoreKitBasicUIKatana");
	UPROPERTY(Config, EditAnywhere, Category = "Damage numbers") FName ElectricStyle = TEXT("CoreKitBasicUIZap");
	UPROPERTY(Config, EditAnywhere, Category = "Damage numbers") FName ElectricLabelStyle = TEXT("CoreKitBasicUIZapLabel");
	UPROPERTY(Config, EditAnywhere, Category = "Damage numbers") FName PierceLabelStyle = TEXT("CoreKitBasicUIPierce");
};

/** Damaged-bar value with the white lag chunk: the fill drops in 90 ms, the chunk holds 300 ms and drains in 420 ms. */
USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUILagBar
{
	GENERATED_BODY()

	static constexpr float DropSeconds = 0.09f;
	static constexpr float HoldSeconds = 0.3f;
	static constexpr float DrainSeconds = 0.42f;

	/** Displayed fill (0..1). */
	UPROPERTY(BlueprintReadOnly) float Fill = 1.f;
	/** White chunk end (>= Fill). */
	UPROPERTY(BlueprintReadOnly) float Chunk = 1.f;
	UPROPERTY(BlueprintReadOnly) float Target = 1.f;

	float FillFrom = 1.f;
	float FillAge = 1.f;
	float ChunkFrom = 1.f;
	float ChunkAge = 1.f;

	void Reset(float Value);
	void SetTarget(float Value);
	void Tick(float DeltaSeconds);
	bool IsSettled() const { return FMath::IsNearlyEqual(Fill, Target) && FMath::IsNearlyEqual(Chunk, Target); }
};

/** Pure helpers shared by the world-signal component, the layer and the tests. */
namespace CoreKitBasicUIWorldSignals
{
	/** Source of a confirmed hit on an enemy: electric element first, then the instigator's active weapon kind. */
	COREKITBASICUI_API ECoreKitBasicUIDamageSource ClassifyDamage(bool bElectric, bool bInstigatorHoldsMelee, bool bFromPlayer);
	/** Floating-text style for the number. */
	COREKITBASICUI_API FName NumberStyle(ECoreKitBasicUIDamageSource Source, const UCoreKitBasicUIWorldSignalSettings* Settings);
	/** PIERCE label: rifle hits while the Piercing Rounds special is owned. */
	COREKITBASICUI_API bool ShowsPierce(ECoreKitBasicUIDamageSource Source, bool bHasPiercing);
	/** Projected diameter of the CHARGING ring in px, clamped. FocalPx = (viewport width / 2) / tan(FOV / 2). */
	COREKITBASICUI_API float ChargeRingDiameter(float WorldRadius, float Distance, float FocalPx, float MinPx, float MaxPx);
	/** Bar opacity from seconds since the last hit (1 while lingering, then a 0.25 s fade). */
	COREKITBASICUI_API float HealthBarOpacity(float SecondsSinceHit, float Linger);
	/** Off-screen edge placement: returns +1 for the right edge, -1 for the left one, from a camera-relative yaw (deg). */
	COREKITBASICUI_API float EdgeSide(float RelativeYawDeg);
	/** World XY -> plan UV (0..1, north = +X is up) for a minimap plan covering [Min, Max]. */
	COREKITBASICUI_API FVector2D WorldToPlanUV(const FVector2D& World, const FVector2D& Min, const FVector2D& Max);
}
