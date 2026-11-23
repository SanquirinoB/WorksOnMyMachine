#pragma once

#include "CoreMinimal.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"

/**
 * Pure helpers and spec timings of the in-run HUD (Docs/UI/Spec/02-HUD.md, 03-HUDEvents.md, 07-Motion.md).
 * No UObject state: used by the HUD widgets and by the CoreKitBasicUI.UI.HUD.* automation tests.
 */
namespace CoreKitBasicUIHudTiming
{
	// 03-HUDEvents / 07-Motion table (the Motion_System table wins over scripts).
	inline constexpr float HitMarker = 0.16f;          // SNAP
	inline constexpr float KillConfirm = 0.26f;        // SNAP
	inline constexpr float KillPop = 0.26f;            // counter 1.25 -> 1
	inline constexpr float DamageFlash = 0.42f;        // SNAP, peak at 12 %
	inline constexpr float DamageFlashPeak = 0.12f;
	inline constexpr float DamageShake = 0.18f;
	inline constexpr float DamageArc = 1.2f;           // holds until 65 %
	inline constexpr float DamageArcHold = 0.65f;
	inline constexpr float HealthDrop = 0.08f;
	inline constexpr float LagDelay = 0.3f;            // white chunk waits
	inline constexpr float LagDrain = 0.42f;           // then drains (EXIT curve)
	inline constexpr float NumberJitter = 0.22f;
	inline constexpr float HealCountUp = 0.35f;
	inline constexpr float BarInterp = 0.3f;           // SETTLE
	inline constexpr float BarGlint = 0.6f;
	inline constexpr float FeedEnter = 0.2f;
	inline constexpr float FeedLife = 1.8f;
	inline constexpr float FeedFadeFrom = 0.82f;       // fraction of the life
	inline constexpr float FeedMergeWindow = 1.f;
	inline constexpr int32 FeedMaxLines = 3;
	inline constexpr float AmmoRoll = 0.25f;
	inline constexpr float CrosshairMorph = 0.16f;     // SETTLE
	inline constexpr float SwapName = 1.f;
	inline constexpr float SwapSlash = 0.5f;
	inline constexpr float SpeedLines = 0.3f;
	inline constexpr float DashPing = 0.42f;
	inline constexpr float StageBanner = 2.6f;
	inline constexpr float StageTrackFill = 0.6f;
	inline constexpr float BossInboundBlink = 0.5f;
	inline constexpr float BossTapeScroll = 0.6f;
	inline constexpr float BossBarDrop = 0.3f;         // SETTLE
	inline constexpr float BossBarGrow = 0.7f;         // DEAL
	inline constexpr float LevelUpFillDelay = 0.12f;
	inline constexpr float LevelUpFill = 0.45f;
	inline constexpr float CardFlightDelay = 0.25f;    // winner rises before it flies (04-Overlays)
	inline constexpr float CardFlight = 0.5f;          // DEAL (drawn by Card Draw)
	inline constexpr float CardLand = 0.42f;           // SLAM
	inline constexpr float CardNewTag = 0.7f;          // from flight start
	inline constexpr float CardGlowStart = 0.9f;
	inline constexpr float CardGlow = 4.f;
	inline constexpr float HudDim = 0.25f;             // seconds to dim / undim
	inline constexpr float HudDimOpacity = 0.25f;
	inline constexpr float HandDimOpacity = 0.35f;
	inline constexpr float LowHealthFraction = 0.3f;
	inline constexpr float LowAmmoFraction = 0.25f;
}

/** One running timeline (real seconds). Tick returns true while running, including the frame that reaches the end. */
struct FCoreKitBasicUIHudTimer
{
	float Time = 0.f;
	float Duration = 0.f;
	bool bRunning = false;

	void Start(float InDuration) { Time = 0.f; Duration = FMath::Max(InDuration, KINDA_SMALL_NUMBER); bRunning = true; }
	void Stop() { bRunning = false; }
	bool Tick(float Dt)
	{
		if (!bRunning)
		{
			return false;
		}
		Time += Dt;
		if (Time >= Duration)
		{
			Time = Duration;
			bRunning = false;
		}
		return true;
	}
	float Alpha() const { return Duration > 0.f ? FMath::Clamp(Time / Duration, 0.f, 1.f) : 1.f; }
	bool IsRunning() const { return bRunning; }
};

/**
 * White "lag chunk" of a damaged bar (07-Motion rule 3): when the value drops the chunk waits 300 ms at the old value,
 * then drains to the new value in 420 ms with the EXIT curve. Rising values move it at once.
 */
struct COREKITBASICUI_API FCoreKitBasicUILagChunk
{
	float Value = 1.f;
	float Target = 1.f;
	float Hold = -1.f;
	FCoreKitBasicUITween Drain;

	void Snap(float InValue);
	void SetTarget(float InTarget);
	void Tick(float Dt);
};

/** A pickup feed line (03-HUDEvents #6). */
struct FCoreKitBasicUIPickupFeedLine
{
	ECoreKitBasicUIPickupKind Kind = ECoreKitBasicUIPickupKind::XP;
	float Amount = 0.f;
	int32 Count = 0;
	/** Seconds since the line was created (reset to the end of the entry on merge). */
	float Age = 0.f;
	/** Seconds since the last merge (merges happen within FeedMergeWindow). */
	float SinceMerge = 0.f;
	int32 Serial = 0;
};

/** Feed model: same-type pickups within 1 s merge (total + count), lines live 1.8 s, at most 3 lines (oldest first). */
struct COREKITBASICUI_API FCoreKitBasicUIPickupFeedModel
{
	TArray<FCoreKitBasicUIPickupFeedLine> Lines;
	int32 NextSerial = 1;

	/** Returns the index of the line that received the pickup. */
	int32 Add(ECoreKitBasicUIPickupKind Kind, float Amount);
	void Tick(float Dt);
	static FText LineText(const FCoreKitBasicUIPickupFeedLine& Line);
};

/** One card slot in the hand fan (02-HUD § Mano de build). */
struct FCoreKitBasicUIFanSlot
{
	/** Left edge of the 52 x 72 card inside the fan box (cards overlap 6 px). */
	float X = 0.f;
	float AngleDeg = 0.f;
	float OffsetY = 0.f;
};

namespace CoreKitBasicUIHudLogic
{
	/** "mm:ss". bCeil rounds up (countdowns show 00:01 until the last second ends). */
	COREKITBASICUI_API FString FormatClock(float Seconds, bool bCeil = true);

	/** Production run duration before a world/view model is available (main menu). */
	COREKITBASICUI_API float DefaultSurvivalSeconds();

	/** Default stage starts, scaled to the configured run duration, when the Run ViewModel has none. */
	COREKITBASICUI_API TArray<float> DefaultStageStarts();

	/** Fill 0..1 of the 4 stage track segments for an elapsed time. Starts = 5 boundaries (missing = defaults). */
	COREKITBASICUI_API void StageTrackFills(const TArray<float>& Starts, float Elapsed, float OutFills[4]);

	/** Health bar: 10 segments; full = 1, partial (shown at 0.45 opacity) = 0..1, empty = 0. */
	COREKITBASICUI_API float SegmentFill(float Fraction, int32 Segment, int32 SegmentCount = 10);

	/** Fan of N mini cards: 5 degree steps around 0, slight arc, newest lifted 8 px. */
	COREKITBASICUI_API TArray<FCoreKitBasicUIFanSlot> FanLayout(int32 Count, int32 NewestIndex = INDEX_NONE, float NewestLift = 8.f);
	COREKITBASICUI_API float FanWidth(int32 Count);
	/** Hand block width: 600 px with up to 5 cards, +40 px per extra card (640 with 6, 680 with 7). */
	COREKITBASICUI_API float HandBlockWidth(int32 Count);

	/**
	 * Direction of a world point seen from the player, in arc degrees (0 = straight ahead / top of the ring,
	 * clockwise, 0..360). Uses the UE convention: yaw 0 looks along +X, positive yaw turns toward +Y.
	 */
	COREKITBASICUI_API float RelativeDirectionDeg(const FVector2D& PlayerPosition, float PlayerYawDeg, const FVector2D& Target);

	/**
	 * World XY to a point of the minimap frame (0..FramePx, north up). North = +X (top of the plan texture),
	 * east = +Y (right). The square frame covers the larger side of the bounds, centred.
	 */
	COREKITBASICUI_API FVector2D WorldToMap(const FVector2D& World, const FVector2D& BoundsMin, const FVector2D& BoundsMax, float FramePx);
	/** World length (cm) to minimap px for the same bounds. */
	COREKITBASICUI_API float WorldLengthToMap(float Length, const FVector2D& BoundsMin, const FVector2D& BoundsMax, float FramePx);

	/** "Stage n, m minutes s seconds until Heavy Mech" (accessible text of the timer). */
	COREKITBASICUI_API FText TimerAccessibleText(int32 StageIndex, float RemainingSeconds);

	/** Number of the stage as shown in the HUD ("STAGE 3", no leading zero) and in banners ("03"). */
	COREKITBASICUI_API FText StageLabel(int32 StageIndex);
	COREKITBASICUI_API FText StageNumber2(int32 StageIndex);

	/** Colour of the summed effect in the TAB panel: white SIGHT stats, mint PLATE stats, magenta otherwise. */
	COREKITBASICUI_API FLinearColor SummedEffectColor(const FCoreKitBasicUICardData& Card);

	/** True when the card id names the Static Dash special (white shock ring on dash). */
	COREKITBASICUI_API bool IsStaticDash(const FName& CardId);

	/** "n DRONES" / "1 DRONE". */
	COREKITBASICUI_API FText DroneCountText(int32 Count);
}
