#pragma once

#include "GameplayTagContainer.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIHudRoot.generated.h"

class UAudioComponent;
class UCanvasPanel;
class UCanvasPanelSlot;
class UCoreKitBasicUIHudBanner;
class UCoreKitBasicUIHudBlock;
class UCoreKitBasicUIHudBossBar;
class UCoreKitBasicUIHudBossWarning;
class UCoreKitBasicUIHudBuildHand;
class UCoreKitBasicUIHudBuildInspector;
class UCoreKitBasicUIHudCrosshair;
class UCoreKitBasicUIHudDamageArcs;
class UCoreKitBasicUIHudDashRing;
class UCoreKitBasicUIHudHealthBlock;
class UCoreKitBasicUIHudKillCounter;
class UCoreKitBasicUIHudMinimap;
class UCoreKitBasicUIHudPickupFeed;
class UCoreKitBasicUIHudScreenFx;
class UCoreKitBasicUIHudSurvivedChip;
class UCoreKitBasicUIHudThreatIndicator;
class UCoreKitBasicUIHudTimerBlock;
class UCoreKitBasicUIHudWeaponBlock;
class UCoreKitBasicUIHudWeaponSwap;
class UCoreKitBasicUIEventChannel;
class UImage;
class UWidget;
struct FInstancedStruct;

/**
 * All HUD v2 A blocks and HUD event layers (UI-2 / UI-5 / UI-6). Created by UCoreKitBasicUIPrimaryLayout and stretched over the
 * screen; Setup receives the layout's full-screen and title-safe canvases (kept for reference: the HUD builds its own
 * layers so it can dim them for Card Draw, scale them with HUD scale and hide them at the end of the run).
 *
 * Layers: ScreenFx (vignettes, flash, speed lines, boss tint) < Main (block vignettes, centre, safe-area blocks; dims to
 * 0.25 while a card offer is open) < Hand (build hand + TAB panel; dims to 0.35). HUD scale scales every block around
 * its anchor on the 96 / 54 title-safe area. Events come from UCoreKitBasicUIEventChannel, data from the UI context ViewModels.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudRoot : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	virtual void Setup(UCanvasPanel* InScreenCanvas, UCanvasPanel* InSafeCanvas);

	/** Routes one UI event to the HUD layers (also used by tests). */
	void HandleEvent(FGameplayTag Event, const FInstancedStruct& Payload);

	UCoreKitBasicUIHudCrosshair* GetCrosshair() const { return Crosshair; }
	UCoreKitBasicUIHudPickupFeed* GetPickupFeed() const { return PickupFeed; }
	UCoreKitBasicUIHudBanner* GetBanner() const { return Banner; }
	UCoreKitBasicUIHudBuildHand* GetHand() const { return Hand; }
	float GetAppliedHudScale() const { return AppliedScale; }
	float GetMainOpacity() const { return MainDim.Value; }
	float GetHandOpacity() const { return HandDim.Value; }

protected:
	void BuildTree() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void TickMotion(float RealDeltaSeconds) override;

	template <typename T>
	T* AddBlock(UCanvasPanel* Layer, FName Name, FVector2D Anchor, FVector2D Position, bool bScaled = true, int32 ZOrder = 0);
	UImage* AddVignette(FVector2D Anchor, FVector2D Extent, float Alpha);
	void ApplyHudScale(float Scale);
	void UpdateLoops(bool bLowHealth, bool bAlarm);
	void SetLoop(TObjectPtr<UAudioComponent>& Component, FName Cue, bool bPlay, float VolumeMultiplier = 1.f);
	bool HasStaticDash() const;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> MainLayer;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> HandLayer;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SafeMain;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SafeHand;
	UPROPERTY(Transient) TObjectPtr<UImage> TopCenterVignette;

	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudScreenFx> ScreenFx;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudKillCounter> KillCounter;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudSurvivedChip> SurvivedChip;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudTimerBlock> Timer;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudBossBar> BossBar;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudBanner> Banner;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudMinimap> Minimap;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudHealthBlock> Health;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudDashRing> Dash;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudWeaponBlock> Weapon;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudPickupFeed> PickupFeed;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudWeaponSwap> WeaponSwap;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudCrosshair> Crosshair;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudDamageArcs> DamageArcs;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudBossWarning> BossWarning;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudThreatIndicator> ThreatLeft;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudThreatIndicator> ThreatRight;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudBuildHand> Hand;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudBuildInspector> Inspector;
	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIHudBlock>> Blocks;

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> HeartbeatLoop;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> AlarmLoop;

	TWeakObjectPtr<UCanvasPanel> LayoutScreenCanvas;
	TWeakObjectPtr<UCanvasPanel> LayoutSafeCanvas;
	TWeakObjectPtr<UCoreKitBasicUIEventChannel> Channel;
	FDelegateHandle EventHandle;

	struct FScaledBlock
	{
		TWeakObjectPtr<UWidget> Widget;
		FVector2D Position = FVector2D::ZeroVector;
	};
	TArray<FScaledBlock> ScaledBlocks;
	float AppliedScale = 1.f;

	FCoreKitBasicUITween MainDim;
	FCoreKitBasicUITween HandDim;
	FCoreKitBasicUITween Visible;
	bool bDimmed = false;
	bool bHidden = false;
	bool bLastOffer = false;
	bool bLastBoss = false;
	double Clock = 0.0;
	double LastHitCue = -10.0;
	double LastHealFeed = -10.0;
	double LastLevelUp = -10.0;
	float LastHealAmount = -1.f;
};
