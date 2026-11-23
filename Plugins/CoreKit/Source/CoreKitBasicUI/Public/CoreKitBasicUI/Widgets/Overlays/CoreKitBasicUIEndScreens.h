#pragma once

#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIEndScreens.generated.h"

class UCanvasPanel;
class UCoreKitBasicUIActionButton;
class UCoreKitBasicUIRunSummaryViewModel;
class UCoreKitBasicUITabularNumber;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UWidget;

/**
 * Base of the end-of-run screens (Docs/UI/Spec/04-Overlays.md § Victoria / Derrota), pushed on the "Menu" layer by
 * UCoreKitBasicUIScreenRouter when the run phase becomes Victory or Defeat. Reads UCoreKitBasicUIRunSummaryViewModel (refreshes when its
 * Serial changes), animates on a real-time clock (the world is paused) and offers RUN AGAIN / RETRY (Enter) and
 * MAIN MENU (Esc) through the Run ViewModel commands.
 */
UCLASS(Abstract)
class COREKITBASICUI_API UCoreKitBasicUIEndScreen : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIEndScreen(const FObjectInitializer& ObjectInitializer);

	virtual bool IsVictory() const { return false; }
	void RunAgain();
	void MainMenu();

	/** Seconds after which the buttons appear and accept input. */
	static constexpr float ButtonsTime = 1.7f;

protected:
	void NativeConstruct() override;
	void TickMotion(float DeltaSeconds) override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;

	/** Fills the data-dependent parts (texts, timeline, build, stat targets). */
	virtual void ApplySummary(const UCoreKitBasicUIRunSummaryViewModel& Summary) {}
	/** Screen-specific motion on the shared clock. */
	virtual void TickSequence(float InClock, float DeltaSeconds) {}

	UCoreKitBasicUIRunSummaryViewModel* GetSummary() const;
	/** Stage boundaries (s) from the Run ViewModel, or the schedule defaults 0/120/300/480/600. */
	TArray<float> StageBounds() const;

	// Builders shared by both screens.
	UWidget* MakeStat(const FText& Label, const FLinearColor& ValueColor, TObjectPtr<UCoreKitBasicUITabularNumber>& OutValue);
	UWidget* MakeButtons(const FText& PrimaryLabel, const FText& PrimarySub);
	void FillCards(UHorizontalBox* Row, const TArray<FCoreKitBasicUICardData>& Cards, bool bVictoryCards, float Gap);

	/** Fade (and rise by Distance px) a widget in at Start over Duration on the screen clock. */
	void AddRise(UWidget* Widget, float Start, float Distance = 16.f, float Duration = 0.52f, ECoreKitBasicUIMotion Curve = ECoreKitBasicUIMotion::Deal);
	/** Count a tabular number from 0 to Target (clock format when bClock, two digits when bTwoDigits). */
	void AddCount(UCoreKitBasicUITabularNumber* Widget, float Start, float Duration, float Target, bool bClock, bool bTwoDigits);
	void ResetCounts();

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UWidget> ButtonGroup;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIActionButton> PrimaryButton;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIActionButton> MenuButton;

	float Clock = 0.f;

private:
	struct FRise
	{
		TWeakObjectPtr<UWidget> Widget;
		float Start = 0.f;
		float Distance = 16.f;
		float Duration = 0.52f;
		ECoreKitBasicUIMotion Curve = ECoreKitBasicUIMotion::Deal;
	};
	struct FCount
	{
		TWeakObjectPtr<UCoreKitBasicUITabularNumber> Widget;
		float Start = 0.f;
		float Duration = 0.6f;
		float Target = 0.f;
		bool bClock = false;
		bool bTwoDigits = false;
	};

	void HandleButton(UCoreKitBasicUIActionButton* Button);

	TArray<FRise> Rises;
	TArray<FCount> Counts;
	int32 SummarySerial = INDEX_NONE;
	bool bActed = false;
};

/** Victory: "RUN COMPLETE · MECH DOWN", timeline with the boss segment, stats, final build (≈ 2.4 s sequence). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIVictoryScreen : public UCoreKitBasicUIEndScreen
{
	GENERATED_BODY()

public:
	bool IsVictory() const override { return true; }

protected:
	void BuildTree() override;
	void ApplySummary(const UCoreKitBasicUIRunSummaryViewModel& Summary) override;
	void TickSequence(float InClock, float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UWidget> Background;
	UPROPERTY(Transient) TObjectPtr<UImage> Flash;
	UPROPERTY(Transient) TObjectPtr<UWidget> Slash;
	UPROPERTY(Transient) TObjectPtr<UWidget> Chip;
	UPROPERTY(Transient) TObjectPtr<UWidget> Title;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Shards;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> TimelineLabels;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> TimelineBar;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> BuildRow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BuildHeader;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> TimeStat;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> KillsStat;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> LevelStat;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> DashesStat;
	bool bJingle = false;
};

/** Defeat: glitch + slow motion, "SIGNAL LOST · DEAD.", timeline with the death marker, final blow tip (≈ 2.2 s). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIDefeatScreen : public UCoreKitBasicUIEndScreen
{
	GENERATED_BODY()

public:
	/** Tip shown when the summary has none (literal of the Defeat mockup for the ranged drone). */
	static FText DefaultTip(const FText& KillerType);

protected:
	void BuildTree() override;
	void NativeDestruct() override;
	void ApplySummary(const UCoreKitBasicUIRunSummaryViewModel& Summary) override;
	void TickSequence(float InClock, float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UWidget> Background;
	UPROPERTY(Transient) TObjectPtr<UImage> Vignette;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Bands;
	UPROPERTY(Transient) TObjectPtr<UWidget> Title;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GhostA;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GhostB;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DownLabel;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> TimelineBar;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> BuildRow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BuildHeader;
	UPROPERTY(Transient) TObjectPtr<UWidget> FinalBlow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FinalBlowTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FinalBlowTip;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> SurvivedStat;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> ShortStat;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> KillsStat;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> LevelStat;

	uint8 Fired = 0;
	bool bSlowMotion = false;
};
