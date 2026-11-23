#pragma once

#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudWidgets.h"
#include "CoreKitBasicUIHudBuild.generated.h"

class UCanvasPanel;
class UCoreKitBasicUICardWidget;
class UCoreKitBasicUIHudKeycap;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/**
 * Bottom centre build hand (02-HUD § Mano de build): "TAB" keycap + fan of 52 x 72 mini cards (UCoreKitBasicUICardWidget Mini),
 * 5 degree steps, 6 px overlap; the newest card lands (event #13: flight drawn by Card Draw, 420 ms SLAM landing,
 * NEW at 700 ms, glow 4 s from 900 ms), stacking pops its card.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudBuildHand : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

public:
	/** CardChosen event (choice time). The flight starts 250 ms later (04-Overlays). */
	void OnCardChosen(const FCoreKitBasicUICardData& Card, bool bStacked);

	/** Centre of the newest card slot in this widget's local space (for a flight drawn by another layer). */
	FVector2D GetNewestSlotCenter() const;

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;
	void RebuildCards(const TArray<FCoreKitBasicUICardData>& InCards);
	void StartLanding(FName CardId, bool bStacked);
	void LayoutCards(float Dt);

	UPROPERTY(Transient) TObjectPtr<USizeBox> BlockBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> FanBox;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Fan;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIHudKeycap> Key;
	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUICardWidget>> CardWidgets;
	UPROPERTY(Transient) TArray<TObjectPtr<USizeBox>> CardBoxes;

	TArray<FCoreKitBasicUICardData> Shown;
	int32 LastPickSerial = INDEX_NONE;
	FName NewestId;

	// Landing sequence of the newest card (seconds from flight start; negative while the winner rises in Card Draw).
	FName AnimId;
	bool bAnimStacked = false;
	bool bAnimating = false;
	float AnimTime = 0.f;
	double AnimStartClock = -10.0;
	bool bLandCue = false;
	bool bNewOn = false;
	FCoreKitBasicUITween Lift;
	double Clock = 0.0;
};

/** Hold TAB: 520 px panel "BUILD · n CARDS" + archetype, one 30 px row per card (thumb, name ×N / SPECIAL, sum). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudBuildInspector : public UCoreKitBasicUIHudBlock
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float Dt) override;
	void RebuildRows(const TArray<FCoreKitBasicUICardData>& InCards);

	UPROPERTY(Transient) TObjectPtr<UWidget> Panel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Header;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Archetype;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Rows;

	TArray<FCoreKitBasicUICardData> LastCards;
	int32 LastCopies = INDEX_NONE;
	bool bOpen = false;
	FCoreKitBasicUIHudTimer Transition;
};
