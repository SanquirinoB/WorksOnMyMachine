#pragma once

#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudWidgets.h"
#include "CoreKitBasicUIHudBlocks.generated.h"

class UCanvasPanel;
class UCoreKitBasicUIArcWidget;
class UCoreKitBasicUIHudKeycap;
class UCoreKitBasicUIMinimapView;
class UCoreKitBasicUITabularNumber;
class UHorizontalBox;
class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UWidget;

/*
 * Persistent HUD v2 A blocks (Docs/UI/Spec/02-HUD.md). Each block reads its ViewModels in TickMotion (continuous
 * values are polled, discrete changes are detected against the last value) and animates with real time.
 */

/** Top left: skull + kills (Saira Condensed 700 26 px, tabular) + "KILLS"; pops 1.25 -> 1 on each kill. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudKillCounter : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void PlayPop();

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> Number;
	FCoreKitBasicUIHudTimer Pop;
	int32 LastKills = INDEX_NONE;
};

/** Boss phase, under the kills: mint chip "10:00 SURVIVED" + overtime "+mm:ss" (02-HUD § Heavy Mech). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudSurvivedChip : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> Chip;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Overtime;
	FCoreKitBasicUIHudTimer Enter;
	bool bShown = false;
};

/** Top centre: "STAGE n" · mm:ss to the mech · "TO MECH", 248 px track with 4 segments 2:3:3:2 and the mech icon. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudTimerBlock : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> StageText;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> Clock;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SegmentFills;

	float Shown[4] = { 0.f, 0.f, 0.f, 0.f };
	FCoreKitBasicUITween SegmentTweens[4];
	int32 LastStage = INDEX_NONE;
	int32 LastSecond = INDEX_NONE;
	/** Fades out when the boss bar takes the slot. */
	FCoreKitBasicUITween Fade;
	bool bBossMode = false;
};

/** Boss bar (720 px) that replaces the timer: tape, name, subtitle, percent, 10 px bar with white lag chunk and ticks. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudBossBar : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> Percent;
	UPROPERTY(Transient) TObjectPtr<UImage> Fill;
	UPROPERTY(Transient) TObjectPtr<UImage> LagImage;
	UPROPERTY(Transient) TObjectPtr<UWidget> Content;

	bool bShown = false;
	FCoreKitBasicUIHudTimer Enter;
	float Displayed = 1.f;
	FCoreKitBasicUILagChunk Lag;
	FCoreKitBasicUITween Grow;
	int32 LastPercent = INDEX_NONE;
};

/** Bottom left 300 x 88: cross + health (52 px) + "/ max", 10 segments with lag chunk, LV chip + XP bar. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudHealthBlock : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	/** Damage taken (event #4): number jitter. The drop and the white chunk follow the ViewModel. */
	void OnDamageTaken();
	/** XP bar flash and fill when a level is reached (event #10). Held full until the offer closes. */
	void PlayLevelUp();

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	void ApplyHealthColors(bool bLow);

	UPROPERTY(Transient) TObjectPtr<UImage> CrossIcon;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> Number;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MaxText;
	UPROPERTY(Transient) TObjectPtr<UWidget> CriticalChip;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SegmentFills;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SegmentLags;
	UPROPERTY(Transient) TObjectPtr<UImage> HealthGlint;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient) TObjectPtr<UImage> XPFill;
	UPROPERTY(Transient) TObjectPtr<UImage> XPGlint;
	UPROPERTY(Transient) TObjectPtr<UImage> XPGlow;

	float LastHealth = -1.f;
	float LastMax = -1.f;
	FCoreKitBasicUITween BarTween;      // health fraction shown by the bar
	FCoreKitBasicUITween NumberTween;   // count-up on heal
	FCoreKitBasicUILagChunk Lag;        // white chunk fraction
	FCoreKitBasicUIHudTimer Jitter;
	FCoreKitBasicUIHudTimer HealGlint;
	bool bLowShown = false;
	float BlinkTime = 0.f;

	float LastXP = -1.f;
	int32 LastLevel = INDEX_NONE;
	FCoreKitBasicUITween XPTween;
	FCoreKitBasicUIHudTimer XPGlintTimer;
	bool bLevelUpHold = false;
	float LevelUpHoldTime = 0.f;
	bool bSawOffer = false;
	FCoreKitBasicUITween XPGlowTween;
};

/** 48 px Dash ring: magenta with glow when ready; grey conic cooldown with seconds; ready ping; Static Dash shock. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudDashRing : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void OnDashed(bool bStaticDash);
	/** Ready ping + HUD.DashReady (deduplicated between the ViewModel edge and the DashReady event). */
	void PlayReady();

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UImage> Glow;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> Ring;
	UPROPERTY(Transient) TObjectPtr<UImage> Chevrons;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Seconds;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> PingRing;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> ShockRing;

	FCoreKitBasicUIHudTimer Ping;
	FCoreKitBasicUIHudTimer Shock;
	bool bLastReady = true;
	double LastReadyTime = -10.0;
	double Clock = 0.0;
};

/** Bottom right 260 x 88: ammo (56 px) + "/ magazine", right-anchored bar, weapon name and N numbered slots. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudWeaponBlock : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;
	void RebuildSlots(const TArray<FCoreKitBasicUIWeaponSlotData>& Slots);

	UPROPERTY(Transient) TObjectPtr<UWidget> LowGroup;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> Ammo;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MagText;
	UPROPERTY(Transient) TObjectPtr<UImage> BarFill;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponName;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SlotCanvas;
	UPROPERTY(Transient) TObjectPtr<USizeBox> SlotBox;
	UPROPERTY(Transient) TObjectPtr<UImage> Underline;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> SlotWidgets;

	TArray<FCoreKitBasicUIWeaponSlotData> LastSlots;
	int32 LastActive = INDEX_NONE;
	FCoreKitBasicUITween UnderlineX;
	FCoreKitBasicUITween BarTween;
	FCoreKitBasicUITween AmmoRoll;
	int32 LastAmmo = INDEX_NONE;
	bool bWasReloading = false;
	bool bLastMelee = false;
	bool bLastLow = false;
};

/** Minimap block: 200 px map + footer (arena name and "n DRONES", or "MECH + n" and "OPTIONAL" during the boss). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudMinimap : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIMinimapView> Map;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LeftText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RightText;

	float PulseTime = 0.f;
	FString LastA11y;
};
