#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudBlocks.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Components/Border.h"
#include "Widgets/HUD/CoreKitBasicUIHudCommon.h"
#include "Engine/Texture2D.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIArcWidget.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIHud"

using namespace CoreKitBasicUIHudBuild;

namespace
{
	/** Large HUD numbers use CSS line-height .78: pull the text box down so the digits sit on the row bottom. */
	FMargin BigNumberPadding(float CssPx, float Right = 8.f)
	{
		// Slate draws text from the top of its box, so moving the box top up is what lifts the digits. Measured in
		// game: with the box at the row top the digit baseline lands ~9-11 px below the 48 px row; this puts it on
		// the row bottom like the mockup (line-height .78, flex-end). Use with VAlign_Fill.
		return FMargin(0.f, -0.2f * CssPx, Right, 0.f);
	}

	bool IsBossPhase(const UCoreKitBasicUIContextSubsystem* Ctx)
	{
		return Ctx && ((Ctx->GetRun() && Ctx->GetRun()->Phase == ECoreKitBasicUIRunPhase::Boss) || (Ctx->GetBoss() && Ctx->GetBoss()->bVisible));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Kill counter

void UCoreKitBasicUIHudKillCounter::BuildTree()
{
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Row"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Row, 0.f, 40.f, TEXT("KillCounter"));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Skull"), CoreKitBasicUIColor::White, 20.f, TEXT("Skull")), FMargin(0.f, 0.f, 8.f, 0.f));
	Number = Tabular(WidgetTree, Sh(D(26.f, 700)), TEXT("Kills"));
	Number->SetValue(TEXT("0"));
	Number->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CoreKitBasicUIBuild::AddH(Row, Number, FMargin(0.f, 0.f, 8.f, 0.f));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Kills", "KILLS"), Sh(M(10.f, 500, CoreKitBasicUIColor::KeyLight, 0.18f)), TEXT("KillsLabel")));
}

void UCoreKitBasicUIHudKillCounter::PlayPop()
{
	Pop.Start(CoreKitBasicUIHudTiming::KillPop);
}

void UCoreKitBasicUIHudKillCounter::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	if (Ctx && Ctx->GetRun())
	{
		const int32 Kills = Ctx->GetRun()->Kills;
		if (Kills != LastKills)
		{
			if (LastKills != INDEX_NONE && Kills > LastKills)
			{
				PlayPop();
			}
			LastKills = Kills;
			Number->SetValue(FString::FromInt(Kills));
		}
	}
	if (Pop.Tick(Dt))
	{
		const float Scale = 1.f + 0.25f * (1.f - Ease(ECoreKitBasicUIMotion::Snap, Pop.Alpha())) * MotionAmount();
		SetXform(Number, FVector2D::ZeroVector, Scale);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// 10:00 SURVIVED chip + overtime

void UCoreKitBasicUIHudSurvivedChip::BuildTree()
{
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Row"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Row, 0.f, 22.f, TEXT("SurvivedChip"));
	UBorder* ChipPlate = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Mint), FMargin(8.f, 0.f), TEXT("ChipPlate"));
	ChipPlate->SetVerticalAlignment(VAlign_Center);
	Chip = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), M(10.f, 700, CoreKitBasicUIColor::Soot, 0.16f), TEXT("Chip"));
	ChipPlate->SetContent(Chip);
	CoreKitBasicUIBuild::AddH(Row, ChipPlate, FMargin(0.f, 0.f, 10.f, 0.f), VAlign_Fill);
	Overtime = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), Sh(M(11.f, 500, CoreKitBasicUIColor::KeyLight, 0.14f)), TEXT("Overtime"));
	CoreKitBasicUIBuild::AddH(Row, Overtime);
	SetRenderOpacity(0.f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCoreKitBasicUIHudSurvivedChip::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const bool bBoss = IsBossPhase(Ctx);
	if (bBoss != bShown)
	{
		bShown = bBoss;
		SetVisibility(bBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bBoss)
		{
			Enter.Start(Token(ECoreKitBasicUIMotion::Settle));
		}
	}
	if (!bShown || !Ctx || !Ctx->GetRun())
	{
		return;
	}
	const UCoreKitBasicUIRunViewModel* Run = Ctx->GetRun();
	SetTextIfChanged(Chip, FText::Format(COREKIT_BASICUI_TEXT("Survived", "{0} SURVIVED"), FText::FromString(CoreKitBasicUIHudLogic::FormatClock(Run->SurvivalSeconds, false))));
	SetTextIfChanged(Overtime, FText::FromString(TEXT("+") + CoreKitBasicUIHudLogic::FormatClock(Run->BossOvertimeSeconds, false)));
	Enter.Tick(Dt);
	const float E = Ease(ECoreKitBasicUIMotion::Settle, Enter.IsRunning() ? Enter.Alpha() : 1.f);
	SetRenderOpacity(E);
	SetXform(GetRootWidget(), FVector2D(0.f, -12.f * (1.f - E) * MotionAmount()));
}

// ---------------------------------------------------------------------------------------------------------------
// Timer + stage track

void UCoreKitBasicUIHudTimerBlock::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Column"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Column, 400.f, 0.f, TEXT("TimerBlock"));

	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("TimerRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Row, 0.f, 40.f), FMargin(0.f), HAlign_Center);
	CoreKitBasicUIBuild::FTextStyle StageStyle = Sh(M(11.f, 700, CoreKitBasicUIColor::Magenta, 0.18f));
	StageStyle.Justify = ETextJustify::Right;
	StageText = CoreKitBasicUIBuild::Text(WidgetTree, CoreKitBasicUIHudLogic::StageLabel(0), StageStyle, TEXT("Stage"));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(WidgetTree, StageText, 72.f, 0.f), FMargin(0.f, 0.f, 14.f, 0.f));
	Clock = Tabular(WidgetTree, Sh(D(40.f, 700, CoreKitBasicUIColor::White, 0.02f)), TEXT("Clock"));
	Clock->SetValue(CoreKitBasicUIHudLogic::FormatClock(CoreKitBasicUIHudLogic::DefaultSurvivalSeconds()));
	CoreKitBasicUIBuild::AddH(Row, Clock, FMargin(0.f, 0.f, 14.f, 0.f));
	UTextBlock* ToMech = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("ToMech", "TO MECH"), Sh(M(11.f, 500, CoreKitBasicUIColor::KeyLight, 0.16f)), TEXT("ToMech"));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(WidgetTree, ToMech, 72.f, 0.f));

	// Track: 248 x 14, segments 2:3:3:2 of 4 px with 3 px gaps, mech icon at the end.
	UHorizontalBox* Track = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("StageTrack"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Track, 248.f, 14.f), FMargin(0.f, 8.f, 0.f, 0.f), HAlign_Center);
	const float Weights[4] = { 2.f, 3.f, 3.f, 2.f };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UOverlay* Segment = CoreKitBasicUIBuild::Overlay(WidgetTree, *FString::Printf(TEXT("Segment%d"), Index));
		CoreKitBasicUIBuild::AddO(Segment, Solid(WidgetTree, CoreKitBasicUIColor::WhiteA(0.3f)));
		UImage* FillImage = Solid(WidgetTree, Index == 3 ? CoreKitBasicUIColor::Orange : CoreKitBasicUIColor::Magenta, *FString::Printf(TEXT("SegmentFill%d"), Index));
		CoreKitBasicUIBuild::AddO(Segment, FillImage);
		SegmentFills.Add(FillImage);
		SetFillX(FillImage, 0.f);
		CoreKitBasicUIBuild::AddH(Track, CoreKitBasicUIBuild::Box(WidgetTree, Segment, 0.f, 4.f), FMargin(Index == 0 ? 0.f : 3.f, 0.f, 0.f, 0.f), VAlign_Center, HAlign_Fill, Weights[Index]);
	}
	CoreKitBasicUIBuild::AddH(Track, CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Mech"), CoreKitBasicUIColor::Orange, 14.f, TEXT("MechIcon")), FMargin(3.f, 0.f, 0.f, 0.f));
	Fade.Snap(1.f);
}

void UCoreKitBasicUIHudTimerBlock::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	if (!Ctx || !Ctx->GetRun())
	{
		return;
	}
	const UCoreKitBasicUIRunViewModel* Run = Ctx->GetRun();

	// The boss bar takes the slot: the timer fades out (300 ms SETTLE) and comes back on a new run.
	const bool bBoss = IsBossPhase(Ctx);
	if (bBoss != bBossMode)
	{
		bBossMode = bBoss;
		Fade.Start(bBoss ? 0.f : 1.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BossBarDrop);
	}
	Fade.Tick(Dt);
	SetRenderOpacity(Fade.Value);
	SetVisibility(Fade.Value <= 0.01f && bBossMode ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (bBossMode && Fade.Value <= 0.01f)
	{
		return;
	}

	const int32 Stage = FMath::Clamp(Run->StageIndex, 0, 3);
	if (Stage != LastStage)
	{
		StageText->SetText(CoreKitBasicUIHudLogic::StageLabel(Stage));
		SetTextColor(StageText, Stage == 3 ? CoreKitBasicUIColor::Orange : CoreKitBasicUIColor::Magenta);
	}

	const int32 Second = FMath::CeilToInt(FMath::Max(0.f, Run->RemainingSeconds) - KINDA_SMALL_NUMBER);
	if (Second != LastSecond || Stage != LastStage)
	{
		LastSecond = Second;
		Clock->SetValue(CoreKitBasicUIHudLogic::FormatClock(Run->RemainingSeconds));
		CoreKitBasicUIHudUtil::SetAccessibleText(this, CoreKitBasicUIHudLogic::TimerAccessibleText(Stage, Run->RemainingSeconds));
	}

	float Fills[4];
	CoreKitBasicUIHudLogic::StageTrackFills(Run->StageStarts, Run->ElapsedSeconds, Fills);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		// Jumps (stage change, retry) fill in 600 ms SETTLE; normal progress follows the clock.
		if (FMath::Abs(Fills[Index] - Shown[Index]) > 0.02f && !SegmentTweens[Index].IsActive() && LastStage != INDEX_NONE)
		{
			SegmentTweens[Index].Start(Shown[Index], Fills[Index], ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::StageTrackFill);
		}
		if (SegmentTweens[Index].IsActive())
		{
			SegmentTweens[Index].To = Fills[Index];
			SegmentTweens[Index].Tick(Dt);
			Shown[Index] = SegmentTweens[Index].Value;
		}
		else
		{
			Shown[Index] = Fills[Index];
		}
		SetFillX(SegmentFills[Index], Shown[Index]);
	}
	LastStage = Stage;
}

// ---------------------------------------------------------------------------------------------------------------
// Boss bar

void UCoreKitBasicUIHudBossBar::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Column"));
	Content = Column;
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Column, 720.f, 0.f, TEXT("BossBar"));

	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("BossRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Row, 0.f, 40.f));
	CoreKitBasicUIBuild::AddH(Row, TapeStrip(WidgetTree, 10, CoreKitBasicUIColor::Orange, 28.f, 28.f, TEXT("BossTape")), FMargin(0.f, 0.f, 12.f, 0.f));
	NameText = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("HeavyMech", "HEAVY MECH"), Sh(D(30.f, 800)), TEXT("BossName"));
	CoreKitBasicUIBuild::AddH(Row, NameText, FMargin(0.f, 0.f, 12.f, 0.f));
	SubtitleText = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("BossSub", "SIEGE UNIT · FINAL TARGET"), Sh(M(10.f, 500, CoreKitBasicUIColor::Orange, 0.2f)), TEXT("BossSubtitle"));
	CoreKitBasicUIBuild::AddH(Row, SubtitleText, FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
	Percent = Tabular(WidgetTree, Sh(D(24.f, 700, CoreKitBasicUIColor::Orange)), TEXT("BossPercent"));
	Percent->SetValue(TEXT("100%"));
	CoreKitBasicUIBuild::AddH(Row, Percent);

	UOverlay* Bar = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("BossHealth"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Bar, 720.f, 10.f), FMargin(0.f, 8.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddO(Bar, Solid(WidgetTree, CoreKitBasicUIColor::SootA(0.7f)));
	LagImage = Solid(WidgetTree, CoreKitBasicUIColor::WhiteA(0.85f), TEXT("BossLag"));
	CoreKitBasicUIBuild::AddO(Bar, LagImage);
	Fill = Solid(WidgetTree, CoreKitBasicUIColor::Orange, TEXT("BossFill"));
	CoreKitBasicUIBuild::AddO(Bar, Fill);
	UCanvasPanel* Ticks = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("BossTicks"));
	CoreKitBasicUIBuild::AddO(Bar, Ticks);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		// 2 px rgba(10,10,12,.9) marks every 72 px.
		CoreKitBasicUIBuild::Place(Ticks, Solid(WidgetTree, CoreKitBasicUIColor::SootA(0.9f)), FAnchors(0.f, 0.f, 0.f, 1.f), FVector2D(70.f + 72.f * Index, 0.f), FVector2D(2.f, 0.f));
		if (UCanvasPanelSlot* TickSlot = Cast<UCanvasPanelSlot>(Ticks->GetChildAt(Index)->Slot))
		{
			TickSlot->SetOffsets(FMargin(70.f + 72.f * Index, 0.f, 2.f, 0.f));
		}
	}
	SetFillX(Fill, 0.f);
	SetFillX(LagImage, 0.f);
	SetRenderOpacity(0.f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCoreKitBasicUIHudBossBar::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIBossViewModel* Boss = Ctx ? Ctx->GetBoss() : nullptr;
	if (!Boss)
	{
		return;
	}
	const bool bBoss = IsBossPhase(Ctx);
	const float Health = FMath::Clamp(Boss->Health, 0.f, 1.f);
	if (bBoss != bShown)
	{
		bShown = bBoss;
		SetVisibility(bBoss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bBoss)
		{
			// At 10:00 the bar drops 12 px in 300 ms SETTLE and grows from 0 in 700 ms DEAL.
			Enter.Start(CoreKitBasicUIHudTiming::BossBarDrop);
			Grow.Start(0.f, Health, ECoreKitBasicUIMotion::Deal, CoreKitBasicUIHudTiming::BossBarGrow);
			Lag.Snap(0.f);
		}
	}
	if (!bShown)
	{
		return;
	}
	if (!Boss->Name.IsEmpty())
	{
		NameText->SetText(Boss->Name);
	}
	if (!Boss->Subtitle.IsEmpty())
	{
		SubtitleText->SetText(Boss->Subtitle);
	}

	Enter.Tick(Dt);
	const float E = Ease(ECoreKitBasicUIMotion::Settle, Enter.IsRunning() ? Enter.Alpha() : 1.f);
	SetRenderOpacity(E);
	SetXform(Content, FVector2D(0.f, -12.f * (1.f - E) * MotionAmount()));

	if (Grow.IsActive())
	{
		Grow.To = Health;
		Grow.Tick(Dt);
		Displayed = Grow.Value;
		Lag.Snap(Displayed);
	}
	else
	{
		Displayed = Health;
		Lag.SetTarget(Health);
	}
	Lag.Tick(Dt);
	SetFillX(Fill, Displayed);
	SetFillX(LagImage, FMath::Max(Lag.Value, Displayed));

	const int32 Pct = FMath::CeilToInt(Health * 100.f - KINDA_SMALL_NUMBER);
	if (Pct != LastPercent)
	{
		LastPercent = Pct;
		Percent->SetValue(FString::Printf(TEXT("%d%%"), FMath::Clamp(Pct, 0, 100)));
		CoreKitBasicUIHudUtil::SetAccessibleText(this, FText::Format(COREKIT_BASICUI_TEXT("BossA11y", "Heavy Mech health {0} percent"), FText::AsNumber(FMath::Clamp(Pct, 0, 100))));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Health + level

void UCoreKitBasicUIHudHealthBlock::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Column"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Column, 300.f, 88.f, TEXT("HealthBlock"));

	// Row of 48: cross, value (52 px tabular), "/ max", HULL CRITICAL chip on the right.
	UHorizontalBox* Top = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("HealthRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Top, 300.f, 48.f));
	CrossIcon = CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Cross"), CoreKitBasicUIColor::Mint, 20.f, TEXT("CrossIcon"));
	CoreKitBasicUIBuild::AddH(Top, CrossIcon, FMargin(0.f, 0.f, 8.f, 4.f), VAlign_Bottom);
	Number = Tabular(WidgetTree, Sh(D(52.f, 700)), TEXT("Health"));
	Number->SetValue(TEXT("100"));
	Number->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CoreKitBasicUIBuild::AddH(Top, Number, BigNumberPadding(52.f), VAlign_Fill);
	MaxText = CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(TEXT("/ 100")), Sh(D(20.f, 600, CoreKitBasicUIColor::TextSoft)), TEXT("MaxHealth"));
	CoreKitBasicUIBuild::AddH(Top, MaxText, FMargin(0.f, 0.f, 0.f, -7.f), VAlign_Bottom, HAlign_Left, 1.f);
	UBorder* Chip = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Red), FMargin(6.f, 2.f), TEXT("HullCritical"));
	Chip->SetContent(CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("HullCritical", "HULL CRITICAL"), M(10.f, 700, CoreKitBasicUIColor::Soot, 0.16f)));
	CriticalChip = Chip;
	CoreKitBasicUIBuild::AddH(Top, Chip, FMargin(0.f, 0.f, 0.f, 6.f), VAlign_Bottom);
	Show(CriticalChip, false);

	// 10 segments of 8 px with 3 px gaps: track, white lag chunk, fill.
	UOverlay* BarStack = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("HealthBar"));
	BarStack->SetClipping(EWidgetClipping::ClipToBounds);
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, BarStack, 300.f, 8.f), FMargin(0.f, 8.f, 0.f, 0.f));
	UHorizontalBox* Segments = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Segments"));
	CoreKitBasicUIBuild::AddO(BarStack, Segments);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		UOverlay* Segment = CoreKitBasicUIBuild::Overlay(WidgetTree, *FString::Printf(TEXT("HealthSegment%d"), Index));
		CoreKitBasicUIBuild::AddO(Segment, Solid(WidgetTree, CoreKitBasicUIColor::WhiteA(0.25f)));
		UImage* LagImage = Solid(WidgetTree, CoreKitBasicUIColor::White, *FString::Printf(TEXT("HealthLag%d"), Index));
		CoreKitBasicUIBuild::AddO(Segment, LagImage);
		UImage* FillImage = Solid(WidgetTree, CoreKitBasicUIColor::Mint, *FString::Printf(TEXT("HealthFill%d"), Index));
		CoreKitBasicUIBuild::AddO(Segment, FillImage);
		SegmentLags.Add(LagImage);
		SegmentFills.Add(FillImage);
		CoreKitBasicUIBuild::AddH(Segments, Segment, FMargin(Index == 0 ? 0.f : 3.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
	}
	UCanvasPanel* GlintCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("HealthGlintCanvas"));
	CoreKitBasicUIBuild::AddO(BarStack, GlintCanvas);
	HealthGlint = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WhiteA(0.95f)), TEXT("HealthGlint"));
	CoreKitBasicUIBuild::Place(GlintCanvas, HealthGlint, FAnchors(0.f, 0.f), FVector2D(0.f, -4.f), FVector2D(72.f, 16.f));
	HealthGlint->SetRenderOpacity(0.f);

	// Row of 16: LV chip + 3 px XP bar.
	UHorizontalBox* Bottom = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("LevelRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Bottom, 300.f, 16.f), FMargin(0.f, 8.f, 0.f, 0.f));
	UBorder* LevelChip = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Lime), FMargin(6.f, 0.f), TEXT("LevelChip"));
	LevelChip->SetVerticalAlignment(VAlign_Center);
	LevelText = CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(TEXT("LV 1")), D(13.f, 800, CoreKitBasicUIColor::Soot), TEXT("Level"));
	LevelChip->SetContent(LevelText);
	CoreKitBasicUIBuild::AddH(Bottom, LevelChip, FMargin(0.f, 0.f, 8.f, 0.f), VAlign_Fill);
	UOverlay* XPStack = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("XPBar"));
	CoreKitBasicUIBuild::AddH(Bottom, CoreKitBasicUIBuild::Box(WidgetTree, XPStack, 0.f, 3.f), FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
	XPGlow = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Lime, 0.8f)), TEXT("XPGlow"));
	CoreKitBasicUIBuild::AddO(XPStack, XPGlow, HAlign_Fill, VAlign_Fill, FMargin(-6.f, -7.f, -6.f, -7.f));
	XPGlow->SetRenderOpacity(0.f);
	CoreKitBasicUIBuild::AddO(XPStack, Solid(WidgetTree, CoreKitBasicUIColor::WhiteA(0.25f)));
	XPFill = Solid(WidgetTree, CoreKitBasicUIColor::Lime, TEXT("XPFill"));
	CoreKitBasicUIBuild::AddO(XPStack, XPFill);
	UCanvasPanel* XPGlintCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("XPGlintCanvas"));
	XPGlintCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	CoreKitBasicUIBuild::AddO(XPStack, XPGlintCanvas);
	XPGlint = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::White), TEXT("XPGlint"));
	CoreKitBasicUIBuild::Place(XPGlintCanvas, XPGlint, FAnchors(0.f, 0.f), FVector2D(0.f, -3.f), FVector2D(60.f, 9.f));
	XPGlint->SetRenderOpacity(0.f);
	SetFillX(XPFill, 0.f);

	BarTween.Snap(1.f);
	NumberTween.Snap(100.f);
	XPTween.Snap(0.f);
	XPGlowTween.Snap(0.f);
}

void UCoreKitBasicUIHudHealthBlock::OnDamageTaken()
{
	if (!IsReducedMotion())
	{
		Jitter.Start(CoreKitBasicUIHudTiming::NumberJitter);
	}
}

void UCoreKitBasicUIHudHealthBlock::PlayLevelUp()
{
	if (bLevelUpHold)
	{
		return;
	}
	bLevelUpHold = true;
	bSawOffer = false;
	LevelUpHoldTime = 0.f;
	// 04-Overlays t=120: the XP bar fills in 450 ms with a flash.
	XPTween.Start(XPTween.Value, 1.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::LevelUpFill, CoreKitBasicUIHudTiming::LevelUpFillDelay);
	XPGlowTween.Start(XPGlowTween.Value, 1.f, ECoreKitBasicUIMotion::Snap, 0.09f, CoreKitBasicUIHudTiming::LevelUpFillDelay);
	XPGlintTimer.Start(CoreKitBasicUIHudTiming::BarGlint);
}

void UCoreKitBasicUIHudHealthBlock::ApplyHealthColors(bool bLow)
{
	const FLinearColor Accent = bLow ? CoreKitBasicUIColor::Red : CoreKitBasicUIColor::Mint;
	CrossIcon->SetColorAndOpacity(Accent);
	Number->SetColor(bLow ? CoreKitBasicUIColor::Red : CoreKitBasicUIColor::White);
	for (UImage* FillImage : SegmentFills)
	{
		FillImage->SetColorAndOpacity(Accent);
	}
	Show(CriticalChip, bLow);
}

void UCoreKitBasicUIHudHealthBlock::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIVitalsViewModel* Vitals = Ctx ? Ctx->GetVitals() : nullptr;
	const UCoreKitBasicUIProgressionViewModel* Progression = Ctx ? Ctx->GetProgression() : nullptr;
	if (!Vitals || !Progression)
	{
		return;
	}

	// --- Health ---------------------------------------------------------------------------------------------
	const float MaxHealth = FMath::Max(Vitals->MaxHealth, 1.f);
	const float Health = FMath::Clamp(Vitals->Health, 0.f, MaxHealth);
	const float Fraction = Health / MaxHealth;
	if (LastHealth < 0.f || !FMath::IsNearlyEqual(MaxHealth, LastMax))
	{
		BarTween.Snap(Fraction);
		NumberTween.Snap(Health);
		Lag.Snap(Fraction);
		MaxText->SetText(FText::FromString(FString::Printf(TEXT("/ %d"), FMath::RoundToInt(MaxHealth))));
		LastMax = MaxHealth;
	}
	else if (Health < LastHealth - KINDA_SMALL_NUMBER)
	{
		// Damage: the value drops in 80 ms and leaves the white chunk.
		Lag.SetTarget(Fraction);
		BarTween.Start(BarTween.Value, Fraction, ECoreKitBasicUIMotion::Snap, CoreKitBasicUIHudTiming::HealthDrop);
		NumberTween.Snap(Health);
	}
	else if (Health > LastHealth + KINDA_SMALL_NUMBER)
	{
		// Heal: count-up in 350 ms, the bar interpolates with a 600 ms glint.
		BarTween.Start(BarTween.Value, Fraction, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::HealCountUp);
		NumberTween.Start(NumberTween.Value, Health, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::HealCountUp);
		HealGlint.Start(CoreKitBasicUIHudTiming::BarGlint + 0.1f);
	}
	LastHealth = Health;

	BarTween.Tick(Dt);
	NumberTween.Tick(Dt);
	const float Bar = BarTween.Value;
	if (Lag.Value < Bar)
	{
		Lag.Snap(Bar);
	}
	Lag.Tick(Dt);

	Number->SetValue(FString::FromInt(FMath::RoundToInt(NumberTween.Value)));
	for (int32 Index = 0; Index < SegmentFills.Num(); ++Index)
	{
		const float FillAmount = CoreKitBasicUIHudLogic::SegmentFill(Bar, Index, SegmentFills.Num());
		// Full segments are opaque; the partial one shows whole at 0.45 (also in low health).
		SegmentFills[Index]->SetVisibility(FillAmount > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		SegmentFills[Index]->SetRenderOpacity(FillAmount >= 0.999f ? 1.f : 0.45f);
		SetFillX(SegmentLags[Index], CoreKitBasicUIHudLogic::SegmentFill(Lag.Value, Index, SegmentLags.Num()));
	}

	const bool bLow = Health > 0.f && (Vitals->bLowHealth || Fraction < CoreKitBasicUIHudTiming::LowHealthFraction);
	if (bLow != bLowShown)
	{
		bLowShown = bLow;
		ApplyHealthColors(bLow);
		BlinkTime = 0.f;
	}
	if (bLowShown)
	{
		// HULL CRITICAL blinks with a 1 s period (1 -> 0.3 -> 1).
		BlinkTime += Dt;
		CriticalChip->SetRenderOpacity(0.65f + 0.35f * FMath::Cos(2.f * PI * BlinkTime / Token(ECoreKitBasicUIMotion::Pulse)));
	}

	// Jitter of the number (220 ms linear: 0, -4, 4, -2, 0).
	if (Jitter.Tick(Dt))
	{
		const float A = Jitter.Alpha();
		static const float Keys[] = { 0.f, -4.f, 4.f, -2.f, 0.f };
		const float Scaled = A * 4.f;
		const int32 K = FMath::Min(FMath::FloorToInt(Scaled), 3);
		const float X = FMath::Lerp(Keys[K], Keys[K + 1], Scaled - K) * MotionAmount();
		SetXform(Number, FVector2D(X, 0.f));
	}

	if (HealGlint.Tick(Dt))
	{
		const float A = Seg(HealGlint.Alpha(), 0.1f / (CoreKitBasicUIHudTiming::BarGlint + 0.1f), 1.f);
		const float E = Ease(ECoreKitBasicUIMotion::Settle, A);
		HealthGlint->SetRenderOpacity(A > 0.f && A < 1.f ? 1.f : 0.f);
		SetXform(HealthGlint, FVector2D((-0.3f + 1.4f * E) * 300.f, 0.f));
	}

	// --- XP and level ---------------------------------------------------------------------------------------
	const int32 Level = Progression->Level;
	if (Level != LastLevel)
	{
		LevelText->SetText(FText::Format(COREKIT_BASICUI_TEXT("Level", "LV {0}"), FText::AsNumber(Level)));
	}
	const float XP = FMath::Clamp(Progression->XPFraction, 0.f, 1.f);
	if (bLevelUpHold)
	{
		LevelUpHoldTime += Dt;
		bSawOffer |= Progression->bOfferOpen;
		const bool bRelease = (bSawOffer && !Progression->bOfferOpen) || (!bSawOffer && LevelUpHoldTime > 3.f);
		if (bRelease)
		{
			bLevelUpHold = false;
			XPTween.Snap(0.f);
			XPTween.Start(0.f, XP, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BarInterp);
			XPGlowTween.Start(XPGlowTween.Value, 0.f, ECoreKitBasicUIMotion::Exit, CoreKitBasicUIHudTiming::BarGlint);
		}
	}
	else if (LastXP < 0.f)
	{
		XPTween.Snap(XP);
	}
	else if (!FMath::IsNearlyEqual(XP, LastXP))
	{
		if (XP > LastXP && Level == LastLevel)
		{
			XPTween.Start(XPTween.Value, XP, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BarInterp);
			XPGlintTimer.Start(CoreKitBasicUIHudTiming::BarGlint);
			XPGlowTween.Start(1.f, 0.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BarGlint);
		}
		else
		{
			XPTween.Start(0.f, XP, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BarInterp);
		}
	}
	LastXP = XP;
	LastLevel = Level;
	XPTween.Tick(Dt);
	XPGlowTween.Tick(Dt);
	SetFillX(XPFill, XPTween.Value);
	XPGlow->SetRenderOpacity(FMath::Clamp(XPGlowTween.Value, 0.f, 1.f));
	if (XPGlintTimer.Tick(Dt))
	{
		const float E = Ease(ECoreKitBasicUIMotion::Settle, XPGlintTimer.Alpha());
		const float Width = XPFill->GetCachedGeometry().GetLocalSize().X > 1.f ? XPFill->GetCachedGeometry().GetLocalSize().X : 250.f;
		XPGlint->SetRenderOpacity(XPGlintTimer.IsRunning() ? 1.f : 0.f);
		SetXform(XPGlint, FVector2D((-0.3f + 1.4f * E) * Width, 0.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Dash ring

void UCoreKitBasicUIHudDashRing::BuildTree()
{
	UOverlay* Root = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("DashStack"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Root, 48.f, 48.f, TEXT("DashRing"));
	// Glow 0 0 12px rgba(255,63,210,.5).
	Glow = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.5f)), TEXT("DashGlow"));
	CoreKitBasicUIBuild::AddO(Root, Glow, HAlign_Fill, VAlign_Fill, FMargin(-16.f));
	Ring = WidgetTree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), TEXT("DashArc"));
	Ring->SetThickness(4.f);
	Ring->SetArcColor(CoreKitBasicUIColor::Magenta);
	CoreKitBasicUIBuild::AddO(Root, Ring);
	CoreKitBasicUIBuild::AddO(Root, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Circle"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Hull, 0.9f), FVector2D(40.f, 40.f))), HAlign_Center, VAlign_Center);
	Chevrons = CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Chevrons"), CoreKitBasicUIColor::Magenta, 18.f, TEXT("DashChevrons"));
	CoreKitBasicUIBuild::AddO(Root, Chevrons, HAlign_Center, VAlign_Center);
	Seconds = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), D(18.f, 700, CoreKitBasicUIColor::TextSoft), TEXT("DashSeconds"));
	CoreKitBasicUIBuild::AddO(Root, Seconds, HAlign_Center, VAlign_Center);
	Show(Seconds, false);
	PingRing = WidgetTree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), TEXT("DashPing"));
	PingRing->SetThickness(2.f);
	PingRing->SetArcColor(CoreKitBasicUIColor::Magenta);
	PingRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PingRing->SetRenderOpacity(0.f);
	CoreKitBasicUIBuild::AddO(Root, PingRing);
	ShockRing = WidgetTree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), TEXT("DashShock"));
	ShockRing->SetThickness(3.f);
	ShockRing->SetArcColor(CoreKitBasicUIColor::White);
	ShockRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	ShockRing->SetRenderOpacity(0.f);
	CoreKitBasicUIBuild::AddO(Root, ShockRing);
}

void UCoreKitBasicUIHudDashRing::OnDashed(bool bStaticDash)
{
	if (bStaticDash)
	{
		Shock.Start(CoreKitBasicUIHudTiming::DashPing);
	}
}

void UCoreKitBasicUIHudDashRing::PlayReady()
{
	if (Clock - LastReadyTime < 0.3)
	{
		return;
	}
	LastReadyTime = Clock;
	Ping.Start(CoreKitBasicUIHudTiming::DashPing);
	PlayHudCue(TEXT("HUD.DashReady"));
}

void UCoreKitBasicUIHudDashRing::TickMotion(float Dt)
{
	Clock += Dt;
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIVitalsViewModel* Vitals = Ctx ? Ctx->GetVitals() : nullptr;
	if (!Vitals)
	{
		return;
	}
	const bool bReady = Vitals->bDashReady || Vitals->DashCharge >= 1.f;
	if (bReady && !bLastReady)
	{
		PlayReady();
	}
	bLastReady = bReady;

	if (bReady)
	{
		Ring->SetArcColor(CoreKitBasicUIColor::Magenta);
		Ring->SetTrackColor(CoreKitBasicUIColor::Transparent);
		Ring->SetProgress(1.f);
	}
	else
	{
		Ring->SetArcColor(CoreKitBasicUIColor::TextSoft);
		Ring->SetTrackColor(CoreKitBasicUIColor::WhiteA(0.2f));
		Ring->SetProgress(FMath::Clamp(Vitals->DashCharge, 0.f, 1.f));
		SetTextIfChanged(Seconds, FText::FromString(FString::Printf(TEXT("%.1f"), FMath::Max(0.f, Vitals->DashCooldownRemaining))));
	}
	Show(Chevrons, bReady);
	Show(Seconds, !bReady);

	float GlowBoost = 0.f;
	if (Ping.Tick(Dt))
	{
		// Ping: ring 1 -> 1.9, 0.9 -> 0 opacity, with a flash of the glow.
		const float E = Ease(ECoreKitBasicUIMotion::Settle, Ping.Alpha());
		PingRing->SetRenderOpacity(0.9f * (1.f - E));
		SetXform(PingRing, FVector2D::ZeroVector, 1.f + 0.9f * E * MotionAmount());
		GlowBoost = 1.f - E;
	}
	Glow->SetRenderOpacity(bReady ? FMath::Clamp(1.f + GlowBoost, 0.f, 1.f) * (1.f + 0.6f * GlowBoost) : 0.f);
	if (Shock.Tick(Dt))
	{
		const float E = Ease(ECoreKitBasicUIMotion::Snap, Shock.Alpha());
		ShockRing->SetRenderOpacity(1.f - E);
		SetXform(ShockRing, FVector2D::ZeroVector, 1.f + 1.2f * E * MotionAmount());
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Ammo + weapons

void UCoreKitBasicUIHudWeaponBlock::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Column"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Column, 260.f, 88.f, TEXT("WeaponBlock"));

	UHorizontalBox* Top = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("AmmoRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Top, 260.f, 48.f));
	CoreKitBasicUIBuild::AddH(Top, Gap(WidgetTree, 1.f), FMargin(0.f), VAlign_Fill, HAlign_Fill, 1.f);
	UHorizontalBox* Low = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("LowAmmo"));
	UCoreKitBasicUIHudKeycap* Key = WidgetTree->ConstructWidget<UCoreKitBasicUIHudKeycap>(UCoreKitBasicUIHudKeycap::StaticClass(), TEXT("ReloadKey"));
	Key->SetAccent(CoreKitBasicUIColor::Magenta, CoreKitBasicUIColor::Magenta);
	if (UInputAction* Reload = CoreKitBasicUIHudUtil::FindInputAction(TEXT("IA_Reload")))
	{
		Key->SetAction(Reload);
	}
	else
	{
		Key->SetKey(EKeys::R);
	}
	CoreKitBasicUIBuild::AddH(Low, Key, FMargin(0.f, 0.f, 6.f, 0.f));
	CoreKitBasicUIBuild::AddH(Low, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Low", "LOW"), M(10.f, 500, CoreKitBasicUIColor::Magenta, 0.16f)));
	LowGroup = Low;
	CoreKitBasicUIBuild::AddH(Top, Low, FMargin(0.f, 0.f, 8.f, 6.f), VAlign_Bottom);
	Show(LowGroup, false);
	Ammo = Tabular(WidgetTree, Sh(D(56.f, 700)), TEXT("Ammo"));
	Ammo->SetValue(TEXT("0"));
	CoreKitBasicUIBuild::AddH(Top, Ammo, BigNumberPadding(56.f), VAlign_Fill);
	MagText = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), Sh(D(20.f, 600, CoreKitBasicUIColor::TextSoft)), TEXT("Magazine"));
	CoreKitBasicUIBuild::AddH(Top, MagText, FMargin(0.f, 0.f, 0.f, -7.f), VAlign_Bottom);

	// 220 x 4 bar anchored right, inside an 8 px row.
	UHorizontalBox* BarRow = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("AmmoBarRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, BarRow, 260.f, 8.f), FMargin(0.f, 8.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddH(BarRow, Gap(WidgetTree, 1.f), FMargin(0.f), VAlign_Fill, HAlign_Fill, 1.f);
	UOverlay* Bar = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("AmmoBar"));
	CoreKitBasicUIBuild::AddO(Bar, Solid(WidgetTree, CoreKitBasicUIColor::WhiteA(0.25f)));
	BarFill = Solid(WidgetTree, CoreKitBasicUIColor::White, TEXT("AmmoFill"));
	CoreKitBasicUIBuild::AddO(Bar, BarFill);
	CoreKitBasicUIBuild::AddH(BarRow, CoreKitBasicUIBuild::Box(WidgetTree, Bar, 220.f, 4.f), FMargin(0.f), VAlign_Center);

	// Row of 16: short weapon name + numbered slots with the sliding magenta underline.
	UHorizontalBox* Bottom = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("WeaponRow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Bottom, 260.f, 16.f), FMargin(0.f, 8.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddH(Bottom, Gap(WidgetTree, 1.f), FMargin(0.f), VAlign_Fill, HAlign_Fill, 1.f);
	WeaponName = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Rifle", "RIFLE"), Sh(M(10.f, 500, CoreKitBasicUIColor::KeyLight, 0.18f)), TEXT("WeaponName"));
	CoreKitBasicUIBuild::AddH(Bottom, WeaponName, FMargin(0.f, 0.f, 10.f, 0.f));
	SlotCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Slots"));
	SlotBox = CoreKitBasicUIBuild::Box(WidgetTree, SlotCanvas, 94.f, 16.f, TEXT("SlotBox"));
	CoreKitBasicUIBuild::AddH(Bottom, SlotBox, FMargin(0.f), VAlign_Fill);
	Underline = Solid(WidgetTree, CoreKitBasicUIColor::Magenta, TEXT("SlotUnderline"));
	UnderlineX.Snap(0.f);
	BarTween.Snap(1.f);
}

void UCoreKitBasicUIHudWeaponBlock::RebuildSlots(const TArray<FCoreKitBasicUIWeaponSlotData>& Slots)
{
	SlotCanvas->ClearChildren();
	SlotWidgets.Reset();
	const int32 Count = FMath::Max(Slots.Num(), 1);
	SlotBox->SetWidthOverride(Count * 44.f + (Count - 1) * 6.f);
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FCoreKitBasicUIWeaponSlotData& Data = Slots[Index];
		UOverlay* SlotStack = CoreKitBasicUIBuild::Overlay(WidgetTree);
		CoreKitBasicUIBuild::AddO(SlotStack, Solid(WidgetTree, CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Hull, 0.7f)));
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddO(SlotStack, Row, HAlign_Center, VAlign_Center);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, FText::AsNumber(Data.SlotNumber), M(10.f, 700, CoreKitBasicUIColor::TextSoft, 0.f)), FMargin(0.f, 0.f, 3.f, 0.f));
		const FName IconName = !Data.Icon.IsNone() ? Data.Icon : (Data.bMelee ? FName(TEXT("T_Icon_Katana")) : FName(TEXT("T_Icon_Rifle")));
		UImage* Icon = CoreKitBasicUIBuild::Icon(WidgetTree, IconName, CoreKitBasicUIColor::White, 24.f);
		UOverlay* IconClip = CoreKitBasicUIBuild::Overlay(WidgetTree);
		CoreKitBasicUIBuild::AddO(IconClip, Icon, HAlign_Center, VAlign_Center, FMargin(0.f, -6.f));
		USizeBox* IconBox = CoreKitBasicUIBuild::Box(WidgetTree, IconClip, 24.f, 12.f);
		IconBox->SetClipping(EWidgetClipping::ClipToBounds);
		CoreKitBasicUIBuild::AddH(Row, IconBox);
		CoreKitBasicUIBuild::Place(SlotCanvas, SlotStack, FAnchors(0.f, 0.f), FVector2D(Index * 50.f, 0.f), FVector2D(44.f, 16.f));
		SlotWidgets.Add(SlotStack);
	}
	CoreKitBasicUIBuild::Place(SlotCanvas, Underline, FAnchors(0.f, 0.f), FVector2D(0.f, 14.f), FVector2D(44.f, 2.f), FVector2D::ZeroVector, 2);
	LastActive = INDEX_NONE;
}

void UCoreKitBasicUIHudWeaponBlock::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUILoadoutViewModel* Loadout = Ctx ? Ctx->GetLoadout() : nullptr;
	if (!Loadout)
	{
		return;
	}
	if (!(Loadout->Slots == LastSlots) || SlotWidgets.Num() != Loadout->Slots.Num())
	{
		LastSlots = Loadout->Slots;
		RebuildSlots(LastSlots);
	}
	int32 Active = LastSlots.IndexOfByPredicate([](const FCoreKitBasicUIWeaponSlotData& Data) { return Data.bActive; });
	if (Active == INDEX_NONE)
	{
		Active = FMath::Clamp(Loadout->ActiveSlot, 0, FMath::Max(LastSlots.Num() - 1, 0));
	}
	const FCoreKitBasicUIWeaponSlotData* ActiveData = LastSlots.IsValidIndex(Active) ? &LastSlots[Active] : nullptr;
	const bool bMelee = Loadout->bMeleeActive || (ActiveData && ActiveData->bMelee);
	if (Active != LastActive)
	{
		// The underline slides in 160 ms SETTLE.
		if (LastActive == INDEX_NONE)
		{
			UnderlineX.Snap(Active * 50.f);
		}
		else
		{
			UnderlineX.Start(UnderlineX.Value, Active * 50.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::CrosshairMorph);
		}
		for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
		{
			SlotWidgets[Index]->SetRenderOpacity(Index == Active ? 1.f : 0.55f);
		}
		FText Short = ActiveData ? ActiveData->ShortName : FText::GetEmpty();
		if (Short.IsEmpty())
		{
			Short = bMelee ? COREKIT_BASICUI_TEXT("Katana", "KATANA") : COREKIT_BASICUI_TEXT("Rifle", "RIFLE");
		}
		WeaponName->SetText(Short.ToUpper());
		LastActive = Active;
	}
	UnderlineX.Tick(Dt);
	SetXform(Underline, FVector2D(UnderlineX.Value, 0.f));
	Show(Underline, SlotWidgets.Num() > 0);

	if (bMelee)
	{
		// Katana: "EDGE" in magenta, full bar, no maximum.
		Ammo->SetValue(TEXT("EDGE"));
		Ammo->SetColor(CoreKitBasicUIColor::Magenta);
		Show(MagText, false);
		Show(LowGroup, false);
		BarFill->SetColorAndOpacity(CoreKitBasicUIColor::Magenta);
		BarTween.Start(BarTween.Value, 1.f, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BarInterp);
		bLastMelee = true;
		LastAmmo = INDEX_NONE;
	}
	else
	{
		const int32 Magazine = FMath::Max(Loadout->MagazineSize, 1);
		const int32 Current = FMath::Max(Loadout->Ammo, 0);
		if (LastAmmo == INDEX_NONE || bLastMelee)
		{
			AmmoRoll.Snap(Current);
		}
		else if (Current != LastAmmo)
		{
			if (Current > LastAmmo && (bWasReloading || Loadout->bReloading))
			{
				// End of reload: the number rolls up in 250 ms.
				AmmoRoll.Start(AmmoRoll.Value, Current, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::AmmoRoll);
			}
			else
			{
				AmmoRoll.Snap(Current);
			}
		}
		LastAmmo = Current;
		bLastMelee = false;
		AmmoRoll.Tick(Dt);
		const bool bLow = Loadout->bLowAmmo || static_cast<float>(Current) < Magazine * CoreKitBasicUIHudTiming::LowAmmoFraction;
		Ammo->SetValue(FString::FromInt(FMath::RoundToInt(AmmoRoll.Value)));
		Ammo->SetColor(bLow ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White);
		SetTextIfChanged(MagText, FText::FromString(FString::Printf(TEXT("/ %d"), Magazine)));
		Show(MagText, true);
		Show(LowGroup, bLow);
		BarFill->SetColorAndOpacity(bLow ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White);
		const float Target = static_cast<float>(Current) / Magazine;
		if (!FMath::IsNearlyEqual(BarTween.To, Target))
		{
			BarTween.Start(BarTween.Value, Target, ECoreKitBasicUIMotion::Settle, CoreKitBasicUIHudTiming::BarInterp);
		}
		bLastLow = bLow;
	}
	bWasReloading = Loadout->bReloading;
	BarTween.Tick(Dt);
	SetFillX(BarFill, BarTween.Value, true);
}

// ---------------------------------------------------------------------------------------------------------------
// Minimap

void UCoreKitBasicUIHudMinimap::BuildTree()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Column"));
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Column, 200.f, 0.f, TEXT("Minimap"));
	Map = WidgetTree->ConstructWidget<UCoreKitBasicUIMinimapView>(UCoreKitBasicUIMinimapView::StaticClass(), TEXT("MinimapView"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(WidgetTree, Map, 200.f, 200.f));
	UHorizontalBox* Footer = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("MinimapFooter"));
	CoreKitBasicUIBuild::AddV(Column, Footer, FMargin(0.f, 6.f, 0.f, 0.f));
	LeftText = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Arena", "ARENA"), Sh(M(10.f, 500, CoreKitBasicUIColor::KeyLight, 0.18f)), TEXT("ArenaName"));
	CoreKitBasicUIBuild::AddH(Footer, LeftText, FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
	RightText = CoreKitBasicUIBuild::Text(WidgetTree, CoreKitBasicUIHudLogic::DroneCountText(0), Sh(M(10.f, 500, CoreKitBasicUIColor::Orange, 0.14f)), TEXT("DroneCount"));
	CoreKitBasicUIBuild::AddH(Footer, RightText);
}

void UCoreKitBasicUIHudMinimap::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIMinimapViewModel* Minimap = Ctx ? Ctx->GetMinimap() : nullptr;
	const UCoreKitBasicUIRunViewModel* Run = Ctx ? Ctx->GetRun() : nullptr;
	const UCoreKitBasicUIBossViewModel* Boss = Ctx ? Ctx->GetBoss() : nullptr;
	if (!Minimap || !Run || !Boss)
	{
		return;
	}
	PulseTime += Dt;
	const float Period = Token(ECoreKitBasicUIMotion::Pulse);
	FCoreKitBasicUIMinimapPaintData Data = Map->GetPaintData();
	if (!Minimap->PlanTexture.IsNull())
	{
		if (!Data.Plan || Data.Plan != Minimap->PlanTexture.Get())
		{
			Data.Plan = Minimap->PlanTexture.LoadSynchronous();
		}
	}
	else
	{
		Data.Plan = nullptr;
	}
	Data.BoundsMin = Minimap->BoundsMin;
	Data.BoundsMax = Minimap->BoundsMax;
	Data.PlayerPosition = Minimap->PlayerPosition;
	Data.PlayerYaw = Minimap->PlayerYaw;
	Data.Entities = Minimap->Entities;
	const bool bBoss = IsBossPhase(Ctx);
	Data.bBossVisible = bBoss;
	Data.BossPosition = Boss->Position;
	Data.AreaCenter = Boss->AreaCenter;
	Data.AreaRadius = bBoss ? Boss->AreaRadius : 0.f;
	Data.Pulse = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Pulse, FMath::Fmod(PulseTime, Period) / Period);
	Map->SetPaintData(Data);

	FText Left;
	FText Right;
	if (bBoss)
	{
		Left = FText::Format(COREKIT_BASICUI_TEXT("MechAdds", "MECH + {0}"), FText::AsNumber(Boss->AddsAlive));
		Right = COREKIT_BASICUI_TEXT("Optional", "OPTIONAL");
		SetTextColor(LeftText, CoreKitBasicUIColor::Orange);
		SetTextColor(RightText, CoreKitBasicUIColor::KeyLight);
	}
	else
	{
		Left = Run->ArenaName.IsEmpty() ? COREKIT_BASICUI_TEXT("Arena", "ARENA") : Run->ArenaName.ToUpper();
		Right = CoreKitBasicUIHudLogic::DroneCountText(Run->AliveDrones);
		SetTextColor(LeftText, CoreKitBasicUIColor::KeyLight);
		SetTextColor(RightText, CoreKitBasicUIColor::Orange);
	}
	SetTextIfChanged(LeftText, Left);
	SetTextIfChanged(RightText, Right);
	const FString A11y = FString::Printf(TEXT("Minimap: %s, %s"), *Left.ToString(), *Right.ToString());
	if (A11y != LastA11y)
	{
		LastA11y = A11y;
		CoreKitBasicUIHudUtil::SetAccessibleText(this, FText::FromString(A11y));
	}
}

#undef LOCTEXT_NAMESPACE
