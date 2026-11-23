#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalTypes.h"

#include "Materials/MaterialInterface.h"

UCoreKitBasicUIWorldSignalSettings::UCoreKitBasicUIWorldSignalSettings()
{
	SlamRingMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/CoreKit/BasicUI/World/Materials/M_WorldSlamRing.M_WorldSlamRing")));
	RepairKitMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/CoreKit/BasicUI/World/Materials/MI_WorldRepairKit.MI_WorldRepairKit")));
	RepairKitRimMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/CoreKit/BasicUI/World/Materials/MI_WorldRepairKitRim.MI_WorldRepairKitRim")));
}

void FCoreKitBasicUILagBar::Reset(float Value)
{
	Fill = Chunk = Target = FillFrom = ChunkFrom = FMath::Clamp(Value, 0.f, 1.f);
	FillAge = DropSeconds;
	ChunkAge = HoldSeconds + DrainSeconds;
}

void FCoreKitBasicUILagBar::SetTarget(float Value)
{
	Value = FMath::Clamp(Value, 0.f, 1.f);
	if (Value >= Fill)
	{
		// Healing (or no change): no chunk, the bar jumps.
		Reset(Value);
		return;
	}
	// The chunk keeps the highest value seen until it drains; a new hit restarts the hold.
	ChunkFrom = FMath::Max(Chunk, Fill);
	Chunk = ChunkFrom;
	ChunkAge = 0.f;
	FillFrom = Fill;
	FillAge = 0.f;
	Target = Value;
}

void FCoreKitBasicUILagBar::Tick(float DeltaSeconds)
{
	DeltaSeconds = FMath::Max(0.f, DeltaSeconds);
	FillAge += DeltaSeconds;
	ChunkAge += DeltaSeconds;
	const float FillAlpha = FMath::Clamp(FillAge / DropSeconds, 0.f, 1.f);
	Fill = FMath::Lerp(FillFrom, Target, FillAlpha);
	const float DrainAlpha = FMath::Clamp((ChunkAge - HoldSeconds) / DrainSeconds, 0.f, 1.f);
	// Ease-out drain (SETTLE-like): fast start, soft landing on the fill.
	const float Eased = 1.f - FMath::Square(1.f - DrainAlpha);
	Chunk = FMath::Max(Fill, FMath::Lerp(ChunkFrom, Target, Eased));
}

namespace CoreKitBasicUIWorldSignals
{
	ECoreKitBasicUIDamageSource ClassifyDamage(bool bElectric, bool bInstigatorHoldsMelee, bool bFromPlayer)
	{
		if (bElectric)
		{
			return ECoreKitBasicUIDamageSource::Electric;
		}
		if (!bFromPlayer)
		{
			return ECoreKitBasicUIDamageSource::Other;
		}
		return bInstigatorHoldsMelee ? ECoreKitBasicUIDamageSource::Katana : ECoreKitBasicUIDamageSource::Rifle;
	}

	FName NumberStyle(ECoreKitBasicUIDamageSource Source, const UCoreKitBasicUIWorldSignalSettings* Settings)
	{
		const UCoreKitBasicUIWorldSignalSettings* S = Settings ? Settings : GetDefault<UCoreKitBasicUIWorldSignalSettings>();
		switch (Source)
		{
		case ECoreKitBasicUIDamageSource::Katana: return S->KatanaStyle;
		case ECoreKitBasicUIDamageSource::Electric: return S->ElectricStyle;
		default: return S->RifleStyle;
		}
	}

	bool ShowsPierce(ECoreKitBasicUIDamageSource Source, bool bHasPiercing)
	{
		return Source == ECoreKitBasicUIDamageSource::Rifle && bHasPiercing;
	}

	float ChargeRingDiameter(float WorldRadius, float Distance, float FocalPx, float MinPx, float MaxPx)
	{
		const float Lo = FMath::Min(MinPx, MaxPx);
		const float Hi = FMath::Max(MinPx, MaxPx);
		if (Distance <= KINDA_SMALL_NUMBER || FocalPx <= 0.f)
		{
			return Hi;
		}
		return FMath::Clamp(2.f * WorldRadius * FocalPx / Distance, Lo, Hi);
	}

	float HealthBarOpacity(float SecondsSinceHit, float Linger)
	{
		if (SecondsSinceHit < 0.f)
		{
			return 0.f;
		}
		constexpr float Fade = 0.25f;
		return 1.f - FMath::Clamp((SecondsSinceHit - Linger) / Fade, 0.f, 1.f);
	}

	float EdgeSide(float RelativeYawDeg)
	{
		return FRotator::NormalizeAxis(RelativeYawDeg) >= 0.f ? 1.f : -1.f;
	}

	FVector2D WorldToPlanUV(const FVector2D& World, const FVector2D& Min, const FVector2D& Max)
	{
		const FVector2D Size(FMath::Max(Max.X - Min.X, 1.f), FMath::Max(Max.Y - Min.Y, 1.f));
		// Image up = +X (north), image right = +Y.
		return FVector2D((World.Y - Min.Y) / Size.Y, 1.f - (World.X - Min.X) / Size.X);
	}
}
