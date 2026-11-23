#pragma once

#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudWidgets.h"
#include "CoreKitBasicUIHudEvents.generated.h"

class UCanvasPanel;
class UCoreKitBasicUIArcWidget;
class UCoreKitBasicUIHudShape;
class UHorizontalBox;
class UImage;
class UOverlay;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/*
 * Transient HUD event layers (Docs/UI/Spec/03-HUDEvents.md). Combat feedback lands at full strength on frame 0 and only
 * animates its exit; travel, shake, jitter and speed lines are skipped under Reduced motion (fades stay).
 */

/**
 * Centre of the screen: crosshair (dot + 4 ticks, or magenta dot + arc with the katana; colour from Preferences),
 * hit marker (#1), kill confirm (#2), reload ring (#7) and the large "R LOW AMMO" warning.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudCrosshair : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void PlayHit(ECoreKitBasicUIDamageSource Source);
	void PlayKill();

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Cross;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> CrossParts;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Blade;
	UPROPERTY(Transient) TObjectPtr<UImage> BladeDot;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> BladeArc;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> HitMarker;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HitTicks;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> KillMarker;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> KillRing;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> ReloadRing;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ReloadLabel;
	UPROPERTY(Transient) TObjectPtr<UWidget> LowAmmo;

	FCoreKitBasicUIHudTimer Hit;
	FCoreKitBasicUIHudTimer Kill;
	FCoreKitBasicUITween Morph;          // 0 = rifle cross, 1 = katana arc
	bool bMelee = false;
	bool bMorphInit = false;
	FLinearColor LastColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	float ReloadVisible = 0.f;
	float LastReloadProgress = 0.f;
};

/** Orange arcs on a 400 px ring pointing at whoever hit the player (#4): 1.2 s, held until 65 %, then fade. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudDamageArcs : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void AddHit(const FVector& SourceLocation);

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	struct FArc
	{
		FVector2D Source = FVector2D::ZeroVector;
		FCoreKitBasicUIHudTimer Life;
	};

	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIArcWidget>> Arcs;
	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIArcWidget>> Glows;
	TArray<FArc> State;
};

/**
 * Full-screen layers: low-health edge vignette pulsing at 1 Hz (#5), red damage flash (#4), dash speed lines (#9) and
 * the warm boss tint (UMG approximation of the post-process tint of 02-HUD § Heavy Mech).
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudScreenFx : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void PlayDamageFlash();
	void PlaySpeedLines();

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UImage> BossTint;
	UPROPERTY(Transient) TObjectPtr<UImage> BossTintTop;
	UPROPERTY(Transient) TObjectPtr<UImage> LowVignette;
	UPROPERTY(Transient) TObjectPtr<UImage> LowDark;
	UPROPERTY(Transient) TObjectPtr<UImage> Flash;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SpeedLines;

	FCoreKitBasicUIHudTimer FlashTimer;
	FCoreKitBasicUIHudTimer Speed;
	FCoreKitBasicUITween LowAmount;
	FCoreKitBasicUITween BossAmount;
	float PulseTime = 0.f;
	bool bLow = false;
	bool bBoss = false;
};

/** Pickup feed above the health block (#6): up to 3 lines, same-type merge within 1 s, 1.8 s life. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudPickupFeed : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void AddPickup(ECoreKitBasicUIPickupKind Kind, float Amount);
	const FCoreKitBasicUIPickupFeedModel& GetModel() const { return Model; }

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Column;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> LineWidgets;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> LineTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> LineCounts;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> LineHeal;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> LineXP;

	FCoreKitBasicUIPickupFeedModel Model;
};

/**
 * Banner slot under the timer (y 132): stage change (#11, 440 px max, 2.6 s, queued while LEVEL UP / Card Draw plays)
 * and the 09:50 "WARNING · HEAVY MECH INBOUND 00:0n" countdown (#12, 520 px, scrolling tape, blinking count).
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudBanner : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	/** Queues a stage banner (deduplicated by stage index between the StageChanged event and the Run ViewModel). */
	void QueueStage(int32 StageIndex, const FText& Name, const FText& Subtitle);
	bool IsBossCountdownActive() const { return bInbound; }
	/** LEVEL UP is the hero: stage banners wait while it plays (07-Motion rule 4). */
	void NotifyLevelUp() { HeroBusy = 1.8f; }

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;
	void ShowStage(int32 StageIndex, const FText& Name, const FText& Subtitle);

	// Stage banner.
	UPROPERTY(Transient) TObjectPtr<UWidget> StageRoot;
	UPROPERTY(Transient) TObjectPtr<UWidget> StageTape;
	UPROPERTY(Transient) TObjectPtr<UWidget> StagePlate;
	UPROPERTY(Transient) TObjectPtr<UImage> StageTapeStripes;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StageNumber;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StageName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StageSubtitle;
	// Boss countdown.
	UPROPERTY(Transient) TObjectPtr<UWidget> InboundRoot;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> InboundTapes;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InboundCount;

	struct FQueued
	{
		int32 Index = 0;
		FText Name;
		FText Subtitle;
	};
	TArray<FQueued> Queue;
	int32 LastQueuedStage = INDEX_NONE;
	int32 LastRunStage = INDEX_NONE;
	FCoreKitBasicUIHudTimer StageLife;
	bool bInbound = false;
	float InboundTime = 0.f;
	FCoreKitBasicUIHudTimer InboundTransition;
	bool bInboundExit = false;
	float HeroBusy = 0.f;
};

/** Boss attack warning centred at y 620 (UI-6): tapes, attack name, instruction and a 260 px telegraph bar. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudBossWarning : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	/** BossTelegraph event: drives the warning for Duration when the Boss ViewModel has no live attack. */
	void OnTelegraph(const FText& AttackName, const FText& Hint, float Duration);

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UWidget> Content;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AttackText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HintText;
	UPROPERTY(Transient) TObjectPtr<UImage> Progress;

	FCoreKitBasicUIHudTimer EventLife;
	FText EventName;
	FText EventHint;
	bool bShown = false;
	FCoreKitBasicUIHudTimer Transition;
};

/**
 * Off-screen lock-on plate (#4): right 96 / top 476 (mirrored on the left edge), conic charge ring, "LOCKING ON" and an
 * orange chevron toward the drone. Fed by ThreatLock events; hidden while the drone is on screen (UI-W draws it there).
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudThreatIndicator : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	void SetThreat(AActor* Source, bool bLocking);
	/** Side for a world point (true = right edge), exposed for tests and the HUD root. */
	static bool IsRightSide(float RelativeDeg) { return RelativeDeg < 180.f; }
	/** Which edge plate this widget is (set by UCoreKitBasicUIHudRoot before construction). */
	void SetRightEdge(bool bInRight) { bRightEdge = bInRight; }

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UWidget> Plate;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> Ring;

	TArray<TWeakObjectPtr<AActor>> Sources;
	bool bRightEdge = true;
	bool bShown = false;
	FCoreKitBasicUIHudTimer Transition;
	float PulseTime = 0.f;
};

/** Weapon swap banner over the ammo module (#8): "n · EQUIPPED", long name revealed by a slash of the weapon colour. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudWeaponSwap : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	/** Shows the banner (deduplicated between the WeaponSwapped event and the Loadout ViewModel). */
	void Play(int32 SlotNumber, const FText& LongName, bool bMelee);

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;

	UPROPERTY(Transient) TObjectPtr<UWidget> Content;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SlotText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<USizeBox> NameClip;
	UPROPERTY(Transient) TObjectPtr<USizeBox> NameFrame;
	UPROPERTY(Transient) TObjectPtr<UImage> Slash;
	UPROPERTY(Transient) TObjectPtr<UImage> SlashGlow;

	FCoreKitBasicUIHudTimer Life;
	float NameWidth = 200.f;
	int32 LastActive = INDEX_NONE;
	int32 LastShownSlot = INDEX_NONE;
	double LastShownClock = -10.0;
	double Clock = 0.0;
};
