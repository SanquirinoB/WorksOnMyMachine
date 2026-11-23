#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"

#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIHud"

void FCoreKitBasicUILagChunk::Snap(float InValue)
{
	Value = Target = InValue;
	Hold = -1.f;
	Drain.Snap(InValue);
}

void FCoreKitBasicUILagChunk::SetTarget(float InTarget)
{
	if (InTarget >= Value - KINDA_SMALL_NUMBER)
	{
		Snap(InTarget);
		return;
	}
	if (FMath::IsNearlyEqual(InTarget, Target) && (Hold > 0.f || Drain.IsActive()))
	{
		return;
	}
	// New damage: the chunk stops where it is and waits again.
	Target = InTarget;
	Hold = CoreKitBasicUIHudTiming::LagDelay;
	Drain.Snap(Value);
}

void FCoreKitBasicUILagChunk::Tick(float Dt)
{
	if (Hold > 0.f)
	{
		Hold -= Dt;
		if (Hold <= 0.f)
		{
			Hold = -1.f;
			Drain.Start(Value, Target, ECoreKitBasicUIMotion::Exit, CoreKitBasicUIHudTiming::LagDrain);
			if (!Drain.IsActive())
			{
				Value = Drain.Value; // Reduced motion snaps
			}
		}
		return;
	}
	if (Drain.IsActive())
	{
		Drain.Tick(Dt);
		Value = Drain.Value;
	}
}

int32 FCoreKitBasicUIPickupFeedModel::Add(ECoreKitBasicUIPickupKind Kind, float Amount)
{
	// Merge into the newest line of the same kind when it received a pickup less than 1 s ago.
	for (int32 Index = Lines.Num() - 1; Index >= 0; --Index)
	{
		FCoreKitBasicUIPickupFeedLine& Line = Lines[Index];
		if (Line.Kind == Kind && Line.SinceMerge < CoreKitBasicUIHudTiming::FeedMergeWindow && Line.Age < CoreKitBasicUIHudTiming::FeedLife)
		{
			Line.Amount += Amount;
			Line.Count += 1;
			Line.SinceMerge = 0.f;
			// Keep it on screen for a full life again, without replaying the entry.
			Line.Age = FMath::Min(Line.Age, CoreKitBasicUIHudTiming::FeedEnter);
			return Index;
		}
	}
	FCoreKitBasicUIPickupFeedLine Line;
	Line.Kind = Kind;
	Line.Amount = Amount;
	Line.Count = 1;
	Line.Serial = NextSerial++;
	Lines.Add(Line);
	while (Lines.Num() > CoreKitBasicUIHudTiming::FeedMaxLines)
	{
		Lines.RemoveAt(0);
	}
	return Lines.Num() - 1;
}

void FCoreKitBasicUIPickupFeedModel::Tick(float Dt)
{
	for (FCoreKitBasicUIPickupFeedLine& Line : Lines)
	{
		Line.Age += Dt;
		Line.SinceMerge += Dt;
	}
	Lines.RemoveAll([](const FCoreKitBasicUIPickupFeedLine& Line) { return Line.Age >= CoreKitBasicUIHudTiming::FeedLife; });
}

FText FCoreKitBasicUIPickupFeedModel::LineText(const FCoreKitBasicUIPickupFeedLine& Line)
{
	const int32 Rounded = FMath::RoundToInt(Line.Amount);
	return Line.Kind == ECoreKitBasicUIPickupKind::Heal
		? FText::Format(COREKIT_BASICUI_TEXT("FeedHeal", "+{0} HP"), FText::AsNumber(Rounded))
		: FText::Format(COREKIT_BASICUI_TEXT("FeedXP", "+{0} XP"), FText::AsNumber(Rounded));
}

namespace CoreKitBasicUIHudLogic
{
	FString FormatClock(float Seconds, bool bCeil)
	{
		const float Clamped = FMath::Max(0.f, Seconds);
		const int32 Total = bCeil ? FMath::CeilToInt(Clamped - KINDA_SMALL_NUMBER) : FMath::FloorToInt(Clamped);
		const int32 Safe = FMath::Max(0, Total);
		return FString::Printf(TEXT("%02d:%02d"), Safe / 60, Safe % 60);
	}

	float DefaultSurvivalSeconds()
	{
		return GetDefault<UCoreKitBasicUISettings>()->DefaultSessionSeconds;
	}

	TArray<float> DefaultStageStarts()
	{
		const float Duration = DefaultSurvivalSeconds();
		return { 0.f, Duration * .2f, Duration * .5f, Duration * .8f, Duration };
	}

	void StageTrackFills(const TArray<float>& InStarts, float Elapsed, float OutFills[4])
	{
		const TArray<float> Starts = InStarts.Num() >= 5 ? InStarts : DefaultStageStarts();
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float From = Starts[Index];
			const float To = Starts[Index + 1];
			OutFills[Index] = To > From ? FMath::Clamp((Elapsed - From) / (To - From), 0.f, 1.f) : (Elapsed >= To ? 1.f : 0.f);
		}
	}

	float SegmentFill(float Fraction, int32 Segment, int32 SegmentCount)
	{
		return FMath::Clamp(FMath::Clamp(Fraction, 0.f, 1.f) * SegmentCount - Segment, 0.f, 1.f);
	}

	float FanWidth(int32 Count)
	{
		// 52 px cards with -3 px margin per side.
		return FMath::Max(0, Count) * 46.f;
	}

	TArray<FCoreKitBasicUIFanSlot> FanLayout(int32 Count, int32 NewestIndex, float NewestLift)
	{
		TArray<FCoreKitBasicUIFanSlot> Slots;
		Slots.Reserve(Count);
		const float Mid = (Count - 1) * 0.5f;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FCoreKitBasicUIFanSlot Slot;
			Slot.X = Index * 46.f - 3.f;
			Slot.AngleDeg = (Index - Mid) * 5.f;
			// Shallow arc: outer cards sit lower (mockup: +8 px at 10 degrees).
			Slot.OffsetY = FMath::Min(0.07f * Slot.AngleDeg * Slot.AngleDeg, 16.f);
			if (Index == NewestIndex)
			{
				Slot.OffsetY -= NewestLift;
			}
			Slots.Add(Slot);
		}
		return Slots;
	}

	float HandBlockWidth(int32 Count)
	{
		return 600.f + 40.f * FMath::Max(0, Count - 5);
	}

	float RelativeDirectionDeg(const FVector2D& PlayerPosition, float PlayerYawDeg, const FVector2D& Target)
	{
		const FVector2D Delta = Target - PlayerPosition;
		if (Delta.IsNearlyZero())
		{
			return 0.f;
		}
		const float WorldDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		float Relative = FMath::Fmod(WorldDeg - PlayerYawDeg, 360.f);
		if (Relative < 0.f)
		{
			Relative += 360.f;
		}
		return Relative;
	}

	FVector2D WorldToMap(const FVector2D& World, const FVector2D& BoundsMin, const FVector2D& BoundsMax, float FramePx)
	{
		const FVector2D Extent = BoundsMax - BoundsMin;
		const float Side = FMath::Max3(static_cast<float>(Extent.X), static_cast<float>(Extent.Y), 1.f);
		const FVector2D Center = (BoundsMin + BoundsMax) * 0.5;
		// North (+X) up, east (+Y) right.
		const float U = 0.5f + (World.Y - Center.Y) / Side;
		const float V = 0.5f - (World.X - Center.X) / Side;
		return FVector2D(FMath::Clamp(U, 0.f, 1.f) * FramePx, FMath::Clamp(V, 0.f, 1.f) * FramePx);
	}

	float WorldLengthToMap(float Length, const FVector2D& BoundsMin, const FVector2D& BoundsMax, float FramePx)
	{
		const FVector2D Extent = BoundsMax - BoundsMin;
		const float Side = FMath::Max3(static_cast<float>(Extent.X), static_cast<float>(Extent.Y), 1.f);
		return Length / Side * FramePx;
	}

	FText TimerAccessibleText(int32 StageIndex, float RemainingSeconds)
	{
		const int32 Total = FMath::Max(0, FMath::CeilToInt(RemainingSeconds - KINDA_SMALL_NUMBER));
		return FText::Format(COREKIT_BASICUI_TEXT("TimerA11y", "Stage {0}, {1} minutes {2} seconds remaining"),
			FText::AsNumber(StageIndex + 1), FText::AsNumber(Total / 60), FText::AsNumber(Total % 60));
	}

	FText StageLabel(int32 StageIndex)
	{
		return FText::Format(COREKIT_BASICUI_TEXT("StageLabel", "STAGE {0}"), FText::AsNumber(StageIndex + 1));
	}

	FText StageNumber2(int32 StageIndex)
	{
		return FText::FromString(FString::Printf(TEXT("%02d"), StageIndex + 1));
	}

	FLinearColor SummedEffectColor(const FCoreKitBasicUICardData& Card)
	{
		if (Card.bSpecial)
		{
			return CoreKitBasicUIColor::Magenta;
		}
		switch (Card.Suit)
		{
		case ECoreKitBasicUICardSuit::Sight: return CoreKitBasicUIColor::White;
		case ECoreKitBasicUICardSuit::Plate: return CoreKitBasicUIColor::Mint;
		default: return CoreKitBasicUIColor::Magenta;
		}
	}

	bool IsStaticDash(const FName& CardId)
	{
		return CardId.ToString().Replace(TEXT(" "), TEXT("")).Contains(TEXT("StaticDash"));
	}

	FText DroneCountText(int32 Count)
	{
		return Count == 1 ? COREKIT_BASICUI_TEXT("OneDrone", "1 DRONE") : FText::Format(COREKIT_BASICUI_TEXT("Drones", "{0} DRONES"), FText::AsNumber(Count));
	}
}

#undef LOCTEXT_NAMESPACE
