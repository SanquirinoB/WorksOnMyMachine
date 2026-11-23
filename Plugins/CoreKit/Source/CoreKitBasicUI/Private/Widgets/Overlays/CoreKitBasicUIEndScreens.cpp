#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIEndScreens.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitBasicUI/Effects/CoreKitBasicUIScreenEffectsComponent.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUIViewModels.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIEndScreens"

namespace
{
	constexpr float TimelineWidth = 1680.f;

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, const CoreKitBasicUIBuild::FTextStyle& Style, FName Name = NAME_None)
	{
		UTextBlock* Block = CoreKitBasicUIBuild::Text(Tree, Text, Style, Name);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	}

	UImage* Solid(UWidgetTree* Tree, const FLinearColor& Color)
	{
		UImage* Image = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(Color));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	UBorder* ChipBox(UWidgetTree* Tree, const FText& Text, const FLinearColor& Fill)
	{
		UBorder* Chip = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(Fill), FMargin(12.f, 6.f));
		Chip->SetContent(Label(Tree, Text, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 700, CoreKitBasicUIColor::Soot, 0.24f)));
		return Chip;
	}

	void AddSpacer(UWidgetTree* Tree, UHorizontalBox* Row)
	{
		CoreKitBasicUIBuild::AddH(Row, Tree->ConstructWidget<USpacer>(USpacer::StaticClass()), FMargin(0.f), VAlign_Fill, HAlign_Fill, 1.f);
	}

	/** Places a full-width (stretched) widget at Y with Height on a canvas. */
	UCanvasPanelSlot* PlaceRow(UCanvasPanel* Canvas, UWidget* Widget, float Left, float Right, float Y, float Height, int32 Z = 0)
	{
		UCanvasPanelSlot* RowSlot = Canvas->AddChildToCanvas(Widget);
		RowSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		RowSlot->SetOffsets(FMargin(Left, Y, Right, Height));
		RowSlot->SetZOrder(Z);
		return RowSlot;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Base

UCoreKitBasicUIEndScreen::UCoreKitBasicUIEndScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowReturn = true;
	bAllowSections = false;
	// The buttons rise at 1.7 s; nothing is accepted before they are visible.
	ActivationInputCooldown = ButtonsTime;
}

UCoreKitBasicUIRunSummaryViewModel* UCoreKitBasicUIEndScreen::GetSummary() const
{
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	return Context ? Context->GetSummary() : nullptr;
}

TArray<float> UCoreKitBasicUIEndScreen::StageBounds() const
{
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	const UCoreKitBasicUIRunViewModel* Run = Context ? Context->GetRun() : nullptr;
	TArray<float> Bounds = Run ? Run->GetStageStarts() : TArray<float>();
	if (Bounds.Num() < 2)
	{
		Bounds = CoreKitBasicUIHudLogic::DefaultStageStarts();
	}
	const float Survival = Run ? Run->GetSurvivalSeconds() : CoreKitBasicUIHudLogic::DefaultSurvivalSeconds();
	if (Bounds.Last() < Survival - 1.f)
	{
		Bounds.Add(Survival);
	}
	return Bounds;
}

void UCoreKitBasicUIEndScreen::NativeConstruct()
{
	if (CoreKitBasicUIMotion::IsReducedMotion())
	{
		ActivationInputCooldown = 0.4f;
	}
	Super::NativeConstruct();
	Clock = 0.f;
	bActed = false;
	if (const UCoreKitBasicUIRunSummaryViewModel* Summary = GetSummary())
	{
		SummarySerial = Summary->GetSerial();
		ApplySummary(*Summary);
	}
	CoreKitBasicUIOverlay::FillButtonContent(PrimaryButton);
	CoreKitBasicUIOverlay::FillButtonContent(MenuButton);
	TickMotion(0.f);
}

UWidget* UCoreKitBasicUIEndScreen::GetInitialFocus_Implementation() const
{
	return PrimaryButton ? PrimaryButton->GetButton() : nullptr;
}

bool UCoreKitBasicUIEndScreen::HandleReturn_Implementation()
{
	if (Clock >= (CoreKitBasicUIMotion::IsReducedMotion() ? 0.4f : ButtonsTime))
	{
		MainMenu();
	}
	return true;
}

void UCoreKitBasicUIEndScreen::RunAgain()
{
	if (bActed)
	{
		return;
	}
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	if (UCoreKitBasicUIRunViewModel* Run = Context ? Context->GetRun() : nullptr; Run && Run->Restart.IsBound())
	{
		bActed = true;
		Run->RequestRestart();
	}
}

void UCoreKitBasicUIEndScreen::MainMenu()
{
	if (bActed)
	{
		return;
	}
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	if (UCoreKitBasicUIRunViewModel* Run = Context ? Context->GetRun() : nullptr)
	{
		bActed = true;
		Run->RequestQuitToMenu();
	}
}

void UCoreKitBasicUIEndScreen::HandleButton(UCoreKitBasicUIActionButton* Button)
{
	if (Button == PrimaryButton)
	{
		RunAgain();
	}
	else if (Button == MenuButton)
	{
		MainMenu();
	}
}

UWidget* UCoreKitBasicUIEndScreen::MakeStat(const FText& LabelText, const FLinearColor& ValueColor, TObjectPtr<UCoreKitBasicUITabularNumber>& OutValue)
{
	UWidgetTree* Tree = WidgetTree;
	UBorder* Tile = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Chamfer(12, CoreKitBasicUIColor::Hull), FMargin(22.f, 18.f));
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(Tree);
	Tile->SetContent(Column);
	CoreKitBasicUIBuild::AddV(Column, Label(Tree, LabelText, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Steel, 0.16f)));
	UCoreKitBasicUITabularNumber* Value = Tree->ConstructWidget<UCoreKitBasicUITabularNumber>(UCoreKitBasicUITabularNumber::StaticClass());
	Value->Setup(CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(52.f), 600, ValueColor));
	CoreKitBasicUIBuild::AddV(Column, Value, FMargin(0.f, 6.f, 0.f, 0.f), HAlign_Left);
	OutValue = Value;
	return Tile;
}

UWidget* UCoreKitBasicUIEndScreen::MakeButtons(const FText& PrimaryLabel, const FText& PrimarySub)
{
	UWidgetTree* Tree = WidgetTree;
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(Tree, TEXT("EndButtons"));
	PrimaryButton = Tree->ConstructWidget<UCoreKitBasicUIActionButton>(UCoreKitBasicUIActionButton::StaticClass(), TEXT("PrimaryButton"));
	PrimaryButton->Setup(PrimaryLabel, PrimarySub, ECoreKitBasicUIButtonVariant::Primary, 78.f, 36.f, true);
	PrimaryButton->OnActivated.AddUObject(this, &ThisClass::HandleButton);
	CoreKitBasicUIBuild::AddV(Column, PrimaryButton);
	MenuButton = Tree->ConstructWidget<UCoreKitBasicUIActionButton>(UCoreKitBasicUIActionButton::StaticClass(), TEXT("MenuButton"));
	MenuButton->Setup(COREKIT_BASICUI_TEXT("MainMenu", "MAIN MENU"), COREKIT_BASICUI_TEXT("Esc", "ESC"), ECoreKitBasicUIButtonVariant::MenuList, 66.f, 30.f, false);
	MenuButton->OnActivated.AddUObject(this, &ThisClass::HandleButton);
	CoreKitBasicUIBuild::AddV(Column, MenuButton, FMargin(0.f, 12.f, 0.f, 0.f));
	USizeBox* Box = CoreKitBasicUIBuild::Box(Tree, Column, 520.f, 0.f);
	CoreKitBasicUIBuild::Place(RootCanvas, Box, FAnchors(1.f, 1.f), FVector2D(-120.f, -120.f), FVector2D::ZeroVector, FVector2D(1.f, 1.f), 20);
	ButtonGroup = Box;
	AddRise(Box, ButtonsTime);
	return Box;
}

void UCoreKitBasicUIEndScreen::FillCards(UHorizontalBox* Row, const TArray<FCoreKitBasicUICardData>& Cards, bool bVictoryCards, float Gap)
{
	if (!Row)
	{
		return;
	}
	Row->ClearChildren();
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		UCoreKitBasicUICardWidget* Card = WidgetTree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass());
		Card->SetVariant(bVictoryCards ? ECoreKitBasicUICardVariant::Victory : ECoreKitBasicUICardVariant::Defeat);
		Card->SetCard(Cards[Index]);
		Card->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::AddH(Row, Card, FMargin(Index > 0 ? Gap : 0.f, 0.f, 0.f, 0.f), VAlign_Bottom);
	}
}

void UCoreKitBasicUIEndScreen::AddRise(UWidget* Widget, float Start, float Distance, float Duration, ECoreKitBasicUIMotion Curve)
{
	if (!Widget)
	{
		return;
	}
	FRise& Rise = Rises.AddDefaulted_GetRef();
	Rise.Widget = Widget;
	Rise.Start = Start;
	Rise.Distance = Distance;
	Rise.Duration = Duration;
	Rise.Curve = Curve;
	Widget->SetRenderOpacity(0.f);
}

void UCoreKitBasicUIEndScreen::AddCount(UCoreKitBasicUITabularNumber* Widget, float Start, float Duration, float Target, bool bClock, bool bTwoDigits)
{
	if (!Widget)
	{
		return;
	}
	Counts.RemoveAll([Widget](const FCount& Count) { return Count.Widget.Get() == Widget; });
	FCount& Count = Counts.AddDefaulted_GetRef();
	Count.Widget = Widget;
	Count.Start = Start;
	Count.Duration = Duration;
	Count.Target = Target;
	Count.bClock = bClock;
	Count.bTwoDigits = bTwoDigits;
}

void UCoreKitBasicUIEndScreen::ResetCounts()
{
	Counts.Reset();
}

void UCoreKitBasicUIEndScreen::TickMotion(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	if (const UCoreKitBasicUIRunSummaryViewModel* Summary = GetSummary(); Summary && Summary->GetSerial() != SummarySerial)
	{
		SummarySerial = Summary->GetSerial();
		ApplySummary(*Summary);
	}
	const bool bReduced = CoreKitBasicUIMotion::IsReducedMotion();
	for (const FRise& Rise : Rises)
	{
		if (UWidget* Widget = Rise.Widget.Get())
		{
			const float Alpha = FMath::Clamp((Clock - Rise.Start) / FMath::Max(0.01f, Rise.Duration), 0.f, 1.f);
			const float E = CoreKitBasicUIMotion::Ease(Rise.Curve, Alpha);
			CoreKitBasicUIOverlay::Pose(Widget, FVector2D(0.f, bReduced ? 0.f : Rise.Distance * (1.f - E)), 1.f, 0.f, FMath::Clamp(E, 0.f, 1.f));
		}
	}
	for (const FCount& Count : Counts)
	{
		if (UCoreKitBasicUITabularNumber* Widget = Count.Widget.Get())
		{
			const float Alpha = FMath::Clamp((Clock - Count.Start) / FMath::Max(0.01f, Count.Duration), 0.f, 1.f);
			const float Value = Count.Target * CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, Alpha);
			const int32 Rounded = Alpha >= 1.f ? FMath::RoundToInt(Count.Target) : FMath::FloorToInt(Value);
			Widget->SetValue(Count.bClock ? CoreKitBasicUIOverlay::Clock(Alpha >= 1.f ? Count.Target : Value)
				: Count.bTwoDigits ? CoreKitBasicUIOverlay::TwoDigits(Rounded) : FString::FromInt(Rounded));
		}
	}
	TickSequence(Clock, DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------------------------
// Victory

void UCoreKitBasicUIVictoryScreen::BuildTree()
{
	UWidgetTree* Tree = WidgetTree;
	RootCanvas = CoreKitBasicUIBuild::Canvas(Tree, TEXT("VictoryRoot"));
	Tree->RootWidget = RootCanvas;

	// Background (opaque Soot + scanlines) fades in under the flash; the huge outlined "10:00" behind everything.
	UCanvasPanel* Bg = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Background"));
	Bg->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoreKitBasicUIBuild::Fill(RootCanvas, Bg, FMargin(0.f), 0);
	CoreKitBasicUIBuild::Fill(Bg, Solid(Tree, CoreKitBasicUIColor::Soot));
	CoreKitBasicUIBuild::Fill(Bg, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Scanlines(0.025f)));
	UTextBlock* BigClock = Label(Tree, FText::FromString(CoreKitBasicUIHudLogic::FormatClock(StageBounds().Last())), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(560.f), 700, CoreKitBasicUIColor::Transparent), TEXT("BigClock"));
	FSlateFontInfo BigFont = BigClock->GetFont();
	BigFont.OutlineSettings.OutlineSize = 2;
	BigFont.OutlineSettings.OutlineColor = CoreKitBasicUIColor::Plate;
	BigFont.OutlineSettings.bSeparateFillAlpha = true;
	BigClock->SetFont(BigFont);
	CoreKitBasicUIBuild::Place(Bg, BigClock, FAnchors(1.f, 0.f), FVector2D(40.f, 40.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f));
	Background = Bg;
	Bg->SetRenderOpacity(0.f);

	// Slash at -5 deg, y 330, 6 px with glow.
	UCanvasPanel* SlashCanvas = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Slash"));
	SlashCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	PlaceRow(RootCanvas, SlashCanvas, -100.f, -300.f, 330.f - 28.f, 62.f, 2);
	CoreKitBasicUIBuild::Fill(SlashCanvas, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.6f))));
	PlaceRow(SlashCanvas, Solid(Tree, CoreKitBasicUIColor::Magenta), 0.f, 0.f, 28.f, 6.f);
	SlashCanvas->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	Slash = SlashCanvas;

	// Boss bar shards (the HUD bar bursts into 8 pieces).
	for (int32 Index = 0; Index < 8; ++Index)
	{
		UImage* Shard = Solid(Tree, Index == 0 ? CoreKitBasicUIColor::Orange : CoreKitBasicUIColor::Seam);
		CoreKitBasicUIBuild::Place(RootCanvas, Shard, FAnchors(0.5f, 0.f), FVector2D(-260.f + Index * 65.f, 74.f), FVector2D(63.f, 8.f), FVector2D::ZeroVector, 25);
		Shards.Add(Shard);
	}

	// Header.
	UVerticalBox* HeaderBox = CoreKitBasicUIBuild::Column(Tree, TEXT("Header"));
	CoreKitBasicUIBuild::Place(RootCanvas, HeaderBox, FAnchors(0.f, 0.f), FVector2D(120.f, 96.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	UHorizontalBox* ChipRow = CoreKitBasicUIBuild::Row(Tree);
	UBorder* RunComplete = ChipBox(Tree, COREKIT_BASICUI_TEXT("RunComplete", "RUN COMPLETE"), CoreKitBasicUIColor::Mint);
	RunComplete->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	CoreKitBasicUIBuild::AddH(ChipRow, RunComplete, FMargin(0.f, 0.f, 12.f, 0.f));
	UTextBlock* Destroyed = Label(Tree, COREKIT_BASICUI_TEXT("MechDestroyed", "HEAVY MECH DESTROYED"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::TextDim, 0.2f));
	CoreKitBasicUIBuild::AddH(ChipRow, Destroyed);
	CoreKitBasicUIBuild::AddV(HeaderBox, ChipRow);
	Chip = RunComplete;
	AddRise(Destroyed, 0.8f, 0.f, 0.3f, ECoreKitBasicUIMotion::Settle);
	UHorizontalBox* TitleRow = CoreKitBasicUIBuild::Row(Tree, TEXT("Title"));
	CoreKitBasicUIBuild::FTextStyle TitleStyle = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(200.f), 700, CoreKitBasicUIColor::White);
	TitleStyle.Tracking = -0.02f;
	CoreKitBasicUIBuild::AddH(TitleRow, Label(Tree, COREKIT_BASICUI_TEXT("Mech", "MECH "), TitleStyle));
	TitleStyle.Color = CoreKitBasicUIColor::Magenta;
	CoreKitBasicUIBuild::AddH(TitleRow, Label(Tree, COREKIT_BASICUI_TEXT("Down", "DOWN"), TitleStyle));
	TitleRow->SetRenderTransformPivot(FVector2D(0.15f, 0.6f));
	// CSS line-height .82: trim Slate's taller line box above and below the title.
	const float TitleTrim = CoreKitBasicUIOverlay::CssLineOffset(UCoreKitBasicUIStyle::Font(ECoreKitBasicUIFontFamily::Display, 700, CoreKitBasicUIBuild::Px(200.f)), 200.f * 0.82f);
	CoreKitBasicUIBuild::AddV(HeaderBox, TitleRow, FMargin(0.f, 12.f + TitleTrim, 0.f, TitleTrim));
	Title = TitleRow;
	UTextBlock* Line = Label(Tree, COREKIT_BASICUI_TEXT("Outdrew", "You outdrew the machine."), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(32.f), 600, CoreKitBasicUIColor::TextDim));
	CoreKitBasicUIBuild::AddV(HeaderBox, Line, FMargin(0.f, 10.f, 0.f, 0.f));
	AddRise(Line, 0.9f, 0.f, 0.3f, ECoreKitBasicUIMotion::Settle);

	// Timeline: labels + 4 magenta stage segments, orange boss segment and a mint end marker.
	UVerticalBox* Timeline = CoreKitBasicUIBuild::Column(Tree, TEXT("Timeline"));
	CoreKitBasicUIBuild::Place(RootCanvas, CoreKitBasicUIBuild::Box(Tree, Timeline, TimelineWidth, 0.f), FAnchors(0.f, 0.f), FVector2D(120.f, 520.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	TimelineLabels = CoreKitBasicUIBuild::Row(Tree, TEXT("TimelineLabels"));
	CoreKitBasicUIBuild::AddV(Timeline, TimelineLabels);
	TimelineBar = CoreKitBasicUIBuild::Row(Tree, TEXT("TimelineBar"));
	CoreKitBasicUIBuild::AddV(Timeline, CoreKitBasicUIBuild::Box(Tree, TimelineBar, 0.f, 14.f), FMargin(0.f, 10.f, 0.f, 0.f));
	AddRise(Timeline, 0.95f, 12.f, 0.52f);

	// Stats.
	UHorizontalBox* Stats = CoreKitBasicUIBuild::Row(Tree, TEXT("Stats"));
	CoreKitBasicUIBuild::Place(RootCanvas, CoreKitBasicUIBuild::Box(Tree, Stats, 1000.f, 0.f), FAnchors(0.f, 0.f), FVector2D(120.f, 620.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	UWidget* StatWidgets[4] = {
		MakeStat(COREKIT_BASICUI_TEXT("TotalTime", "TOTAL TIME"), CoreKitBasicUIColor::White, TimeStat),
		MakeStat(COREKIT_BASICUI_TEXT("Kills", "KILLS"), CoreKitBasicUIColor::White, KillsStat),
		MakeStat(COREKIT_BASICUI_TEXT("FinalLevel", "FINAL LEVEL"), CoreKitBasicUIColor::Lime, LevelStat),
		MakeStat(COREKIT_BASICUI_TEXT("Dashes", "DASHES"), CoreKitBasicUIColor::Magenta, DashesStat) };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		CoreKitBasicUIBuild::AddH(Stats, StatWidgets[Index], FMargin(Index > 0 ? 14.f : 0.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
		AddRise(StatWidgets[Index], 1.0f + Index * 0.07f);
	}

	// Final build.
	UVerticalBox* BuildBox = CoreKitBasicUIBuild::Column(Tree, TEXT("FinalBuild"));
	CoreKitBasicUIBuild::Place(RootCanvas, BuildBox, FAnchors(0.f, 0.f), FVector2D(120.f, 790.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	BuildHeader = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.22f));
	CoreKitBasicUIBuild::AddV(BuildBox, BuildHeader);
	BuildRow = CoreKitBasicUIBuild::Row(Tree, TEXT("BuildCards"));
	CoreKitBasicUIBuild::AddV(BuildBox, BuildRow, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	AddRise(BuildBox, 1.3f);

	Flash = Solid(Tree, FLinearColor::White);
	Flash->SetRenderOpacity(0.f);
	CoreKitBasicUIBuild::Fill(RootCanvas, Flash, FMargin(0.f), 40);

	MakeButtons(COREKIT_BASICUI_TEXT("RunAgain", "RUN AGAIN"), COREKIT_BASICUI_TEXT("Enter", "ENTER"));
}

void UCoreKitBasicUIVictoryScreen::ApplySummary(const UCoreKitBasicUIRunSummaryViewModel& Summary)
{
	UWidgetTree* Tree = WidgetTree;
	const float Total = Summary.GetTotalSeconds();
	ResetCounts();
	AddCount(TimeStat, 1.0f, 0.8f, Total, true, false);
	AddCount(KillsStat, 1.0f, 0.8f, static_cast<float>(Summary.GetKills()), false, false);
	AddCount(LevelStat, 1.0f, 0.8f, static_cast<float>(Summary.GetLevel()), false, true);
	AddCount(DashesStat, 1.0f, 0.8f, static_cast<float>(Summary.GetDashes()), false, false);

	// Timeline labels: 00:00 · stage names · 10:00 · BOSS +mm:ss (spread across the bar).
	const TArray<float> Bounds = StageBounds();
	const CoreKitBasicUIBuild::FTextStyle LabelStyle = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.18f);
	TimelineLabels->ClearChildren();
	TArray<FText> Labels;
	Labels.Add(FText::FromString(CoreKitBasicUIOverlay::Clock(0.f)));
	for (int32 Stage = 0; Stage + 1 < Bounds.Num(); ++Stage)
	{
		Labels.Add(CoreKitBasicUIOverlay::StageName(Stage));
	}
	Labels.Add(FText::FromString(CoreKitBasicUIOverlay::Clock(Bounds.Last())));
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		CoreKitBasicUIBuild::AddH(TimelineLabels, Label(Tree, Labels[Index], LabelStyle));
		AddSpacer(Tree, TimelineLabels);
	}
	const FText BossText = FText::Format(COREKIT_BASICUI_TEXT("BossPlus", "BOSS +{0}"), FText::FromString(CoreKitBasicUIOverlay::Clock(Summary.GetBossSeconds())));
	CoreKitBasicUIBuild::AddH(TimelineLabels, Label(Tree, BossText, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Orange, 0.18f)));

	TimelineBar->ClearChildren();
	for (int32 Stage = 0; Stage + 1 < Bounds.Num(); ++Stage)
	{
		const float Minutes = FMath::Max(0.1f, (Bounds[Stage + 1] - Bounds[Stage]) / 60.f);
		CoreKitBasicUIBuild::AddH(TimelineBar, Solid(Tree, CoreKitBasicUIColor::Magenta), FMargin(Stage > 0 ? 4.f : 0.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, Minutes);
	}
	CoreKitBasicUIBuild::AddH(TimelineBar, Solid(Tree, CoreKitBasicUIColor::Orange), FMargin(4.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, FMath::Max(0.3f, Summary.GetBossSeconds() / 60.f));
	CoreKitBasicUIBuild::AddH(TimelineBar, CoreKitBasicUIBuild::Box(Tree, Solid(Tree, CoreKitBasicUIColor::Mint), 14.f, 0.f), FMargin(4.f, 0.f, 0.f, 0.f), VAlign_Fill);

	// FINAL BUILD · n CARDS · ARCHETYPE
	const FText Cards = FText::Format(COREKIT_BASICUI_TEXT("Cards", "{0} {0}|plural(one=CARD,other=CARDS)"), FText::AsNumber(Summary.GetTotalCopies()));
	BuildHeader->SetText(Summary.GetArchetype().IsEmpty()
		? FText::Format(COREKIT_BASICUI_TEXT("FinalBuild", "FINAL BUILD · {0}"), Cards)
		: FText::Format(COREKIT_BASICUI_TEXT("FinalBuildArch", "FINAL BUILD · {0} · {1}"), Cards, Summary.GetArchetype().ToUpper()));
	FillCards(BuildRow, Summary.GetFinalCards(), true, 10.f);
}

void UCoreKitBasicUIVictoryScreen::TickSequence(float InClock, float DeltaSeconds)
{
	const bool bReduced = CoreKitBasicUIMotion::IsReducedMotion();
	// 0: the boss bar bursts into 8 shards (500 ms SNAP, 15 ms stagger).
	static const FVector Offsets[8] = { {-60, -40, -40}, {-40, 30, 25}, {-20, -60, -15}, {-5, 45, 30}, {10, -50, 20}, {25, 35, -35}, {45, -30, 40}, {60, 50, -25} };
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		const float A = FMath::Clamp((InClock - Index * 0.015f) / 0.5f, 0.f, 1.f);
		const float E = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Snap, A);
		const FVector& O = Offsets[Index];
		const float Travel = bReduced ? 0.f : 2.5f;
		CoreKitBasicUIOverlay::Pose(Shards[Index], FVector2D(O.X, O.Y) * Travel * E, 1.f, bReduced ? 0.f : O.Z * E, 1.f - A);
	}
	// 250: white flash 300 ms (peak .9); the opaque background arrives under it.
	if (Flash)
	{
		const float T = (InClock - 0.25f) / 0.3f;
		const float A = T < 0.f || T > 1.f ? 0.f : (T < 0.2f ? 0.9f * T / 0.2f : 0.9f * (1.f - (T - 0.2f) / 0.8f));
		Flash->SetRenderOpacity(A);
	}
	if (Background)
	{
		Background->SetRenderOpacity(CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, FMath::Clamp((InClock - 0.25f) / 0.3f, 0.f, 1.f)));
	}
	// 450: slash wipes (260 ms SETTLE) and MECH DOWN stamps 1.7 -> 1 (380 ms SLAM).
	if (Slash)
	{
		const float A = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, FMath::Clamp((InClock - 0.45f) / 0.26f, 0.f, 1.f));
		FWidgetTransform T;
		T.Angle = -5.f;
		T.Scale = FVector2D(FMath::Max(0.001f, A), 1.f);
		Slash->SetRenderTransform(T);
		Slash->SetRenderOpacity(InClock >= 0.45f ? 1.f : 0.f);
	}
	if (Title)
	{
		const float A = FMath::Clamp((InClock - 0.45f) / 0.38f, 0.f, 1.f);
		const float Scale = bReduced ? 1.f : FMath::Lerp(1.7f, 1.f, CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Slam, A));
		CoreKitBasicUIOverlay::Pose(Title, FVector2D::ZeroVector, Scale, 0.f, FMath::Clamp(A * 2.f, 0.f, 1.f));
	}
	if (!bJingle && InClock >= 0.45f)
	{
		bJingle = true;
		PlayCue(TEXT("End.VictoryJingle"));
	}
	// 800: chip pop (300 ms).
	if (Chip)
	{
		const float A = FMath::Clamp((InClock - 0.8f) / 0.3f, 0.f, 1.f);
		CoreKitBasicUIOverlay::Pose(Chip, FVector2D::ZeroVector, bReduced ? 1.f : CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Slam, A), 0.f, A > 0.f ? 1.f : 0.f);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Defeat

FText UCoreKitBasicUIDefeatScreen::DefaultTip(const FText& KillerType)
{
	const FString Type = KillerType.ToString().ToUpper();
	if (Type.Contains(TEXT("RANGED")))
	{
		return COREKIT_BASICUI_TEXT("TipRanged", "An orange ring around a drone means it is about to fire. Dash sideways as the ring closes.");
	}
	// Other enemy types: tips pending (Docs/UI/Spec/11-Decisions.md § Pendientes).
	return FText::GetEmpty();
}

void UCoreKitBasicUIDefeatScreen::BuildTree()
{
	UWidgetTree* Tree = WidgetTree;
	RootCanvas = CoreKitBasicUIBuild::Canvas(Tree, TEXT("DefeatRoot"));
	Tree->RootWidget = RootCanvas;

	// Background: dark red tint, red scanlines, glitch bands.
	UCanvasPanel* Bg = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Background"));
	Bg->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoreKitBasicUIBuild::Fill(RootCanvas, Bg, FMargin(0.f), 0);
	CoreKitBasicUIBuild::Fill(Bg, Solid(Tree, CoreKitBasicUIColor::Soot));
	UImage* Tint = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.16f)));
	CoreKitBasicUIBuild::Place(Bg, Tint, FAnchors(0.f, 0.f), FVector2D(-500.f, -300.f), FVector2D(2160.f, 1340.f));
	FSlateBrush RedLines = UCoreKitBasicUIStyle::Image(TEXT("T_UI_Scanlines"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.05f));
	RedLines.Tiling = ESlateBrushTileType::Both;
	CoreKitBasicUIBuild::Fill(Bg, CoreKitBasicUIBuild::Image(Tree, RedLines));
	Background = Bg;
	Bg->SetRenderOpacity(0.f);
	UImage* BandA = Solid(Tree, CoreKitBasicUIColor::WhiteA(0.18f));
	PlaceRow(RootCanvas, BandA, 0.f, 0.f, 300.f, 14.f, 1);
	UImage* BandB = Solid(Tree, CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.22f));
	PlaceRow(RootCanvas, BandB, 0.f, 0.f, 412.f, 6.f, 1);
	Bands = { BandA, BandB };

	// Red vignette: full from frame 0, fades 700 ms after the glitch.
	Vignette = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_EdgeVignette"), CoreKitBasicUIColor::Red));
	Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoreKitBasicUIBuild::Fill(RootCanvas, Vignette, FMargin(0.f), 30);

	// Header: SIGNAL LOST · HULL INTEGRITY 0% / DEAD. with ghosts / subtitle.
	UVerticalBox* HeaderBox = CoreKitBasicUIBuild::Column(Tree, TEXT("Header"));
	CoreKitBasicUIBuild::Place(RootCanvas, HeaderBox, FAnchors(0.f, 0.f), FVector2D(120.f, 90.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	UHorizontalBox* ChipRow = CoreKitBasicUIBuild::Row(Tree);
	CoreKitBasicUIBuild::AddH(ChipRow, ChipBox(Tree, COREKIT_BASICUI_TEXT("SignalLost", "SIGNAL LOST"), CoreKitBasicUIColor::Red), FMargin(0.f, 0.f, 12.f, 0.f));
	CoreKitBasicUIBuild::AddH(ChipRow, Label(Tree, COREKIT_BASICUI_TEXT("Hull0", "HULL INTEGRITY 0%"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::TextDim, 0.2f)));
	CoreKitBasicUIBuild::AddV(HeaderBox, ChipRow);
	AddRise(ChipRow, 0.55f, 0.f, 0.3f, ECoreKitBasicUIMotion::Settle);

	UCanvasPanel* Stamp = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Stamp"));
	CoreKitBasicUIBuild::AddV(HeaderBox, CoreKitBasicUIBuild::Box(Tree, Stamp, 900.f, 290.f), FMargin(0.f, 8.f, 0.f, 0.f));
	const CoreKitBasicUIBuild::FTextStyle DeadStyle = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(300.f), 700, CoreKitBasicUIColor::Red);
	GhostA = Label(Tree, COREKIT_BASICUI_TEXT("Dead", "DEAD."), DeadStyle, TEXT("GhostA"));
	GhostB = Label(Tree, COREKIT_BASICUI_TEXT("Dead", "DEAD."), DeadStyle, TEXT("GhostB"));
	UTextBlock* Main = Label(Tree, COREKIT_BASICUI_TEXT("Dead", "DEAD."), DeadStyle, TEXT("Dead"));
	// The mockup's h1 sits at the top of a 290 px block with line-height .9 (270 px); Slate's line box is taller, so
	// the glyphs are lifted by the difference to keep the subline 8 px under the block.
	const float StampY = CoreKitBasicUIOverlay::CssLineOffset(Main->GetFont(), 300.f * 0.9f);
	CoreKitBasicUIBuild::Place(Stamp, GhostA, FAnchors(0.f, 0.f), FVector2D(0.f, StampY));
	CoreKitBasicUIBuild::Place(Stamp, GhostB, FAnchors(0.f, 0.f), FVector2D(0.f, StampY));
	CoreKitBasicUIBuild::Place(Stamp, Main, FAnchors(0.f, 0.f), FVector2D(0.f, StampY));
	Main->SetRenderTransformPivot(FVector2D(0.2f, 0.6f));
	Title = Main;
	UTextBlock* Line = Label(Tree, COREKIT_BASICUI_TEXT("DrewFirst", "The machine drew first."), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(34.f), 600, CoreKitBasicUIColor::TextDim));
	CoreKitBasicUIBuild::AddV(HeaderBox, Line, FMargin(0.f, 8.f, 0.f, 0.f));
	AddRise(Line, 0.9f, 0.f, 0.3f, ECoreKitBasicUIMotion::Settle);

	// Timeline: "00:00 · DOWN AT mm:ss · STAGE nn · 10:00 · HEAVY MECH", red fill to the death point and a marker.
	UVerticalBox* Timeline = CoreKitBasicUIBuild::Column(Tree, TEXT("Timeline"));
	CoreKitBasicUIBuild::Place(RootCanvas, CoreKitBasicUIBuild::Box(Tree, Timeline, TimelineWidth, 0.f), FAnchors(0.f, 0.f), FVector2D(120.f, 560.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	UHorizontalBox* Labels = CoreKitBasicUIBuild::Row(Tree, TEXT("TimelineLabels"));
	const CoreKitBasicUIBuild::FTextStyle LabelStyle = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.18f);
	CoreKitBasicUIBuild::AddH(Labels, Label(Tree, FText::FromString(CoreKitBasicUIOverlay::Clock(0.f)), LabelStyle));
	AddSpacer(Tree, Labels);
	DownLabel = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Red, 0.18f));
	CoreKitBasicUIBuild::AddH(Labels, DownLabel);
	AddSpacer(Tree, Labels);
	CoreKitBasicUIBuild::AddH(Labels, Label(Tree, FText::Format(COREKIT_BASICUI_TEXT("MechAt", "{0} · HEAVY MECH"), FText::FromString(CoreKitBasicUIHudLogic::FormatClock(StageBounds().Last()))), LabelStyle));
	CoreKitBasicUIBuild::AddV(Timeline, Labels);
	TimelineBar = CoreKitBasicUIBuild::Canvas(Tree, TEXT("TimelineBar"));
	CoreKitBasicUIBuild::AddV(Timeline, CoreKitBasicUIBuild::Box(Tree, TimelineBar, TimelineWidth, 14.f), FMargin(0.f, 10.f, 0.f, 0.f));
	AddRise(Timeline, 1.0f, 12.f, 0.52f);

	// Stats.
	UHorizontalBox* Stats = CoreKitBasicUIBuild::Row(Tree, TEXT("Stats"));
	CoreKitBasicUIBuild::Place(RootCanvas, CoreKitBasicUIBuild::Box(Tree, Stats, 1000.f, 0.f), FAnchors(0.f, 0.f), FVector2D(120.f, 650.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	UWidget* StatWidgets[4] = {
		MakeStat(COREKIT_BASICUI_TEXT("Survived", "SURVIVED"), CoreKitBasicUIColor::White, SurvivedStat),
		MakeStat(COREKIT_BASICUI_TEXT("ShortBy", "SHORT BY"), CoreKitBasicUIColor::Red, ShortStat),
		MakeStat(COREKIT_BASICUI_TEXT("Kills", "KILLS"), CoreKitBasicUIColor::White, KillsStat),
		MakeStat(COREKIT_BASICUI_TEXT("Level", "LEVEL"), CoreKitBasicUIColor::Lime, LevelStat) };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		CoreKitBasicUIBuild::AddH(Stats, StatWidgets[Index], FMargin(Index > 0 ? 14.f : 0.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
		AddRise(StatWidgets[Index], 1.1f + Index * 0.07f);
	}

	// Build at death + final blow card.
	UHorizontalBox* Bottom = CoreKitBasicUIBuild::Row(Tree, TEXT("Bottom"));
	CoreKitBasicUIBuild::Place(RootCanvas, Bottom, FAnchors(0.f, 0.f), FVector2D(120.f, 820.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 10);
	UVerticalBox* BuildBox = CoreKitBasicUIBuild::Column(Tree);
	BuildHeader = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.22f));
	CoreKitBasicUIBuild::AddV(BuildBox, BuildHeader);
	BuildRow = CoreKitBasicUIBuild::Row(Tree, TEXT("BuildCards"));
	CoreKitBasicUIBuild::AddV(BuildBox, BuildRow, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	CoreKitBasicUIBuild::AddH(Bottom, BuildBox, FMargin(0.f, 0.f, 48.f, 0.f), VAlign_Bottom);

	UOverlay* Blow = CoreKitBasicUIBuild::Overlay(Tree, TEXT("FinalBlow"));
	CoreKitBasicUIBuild::AddO(Blow, Solid(Tree, CoreKitBasicUIColor::Hull));
	CoreKitBasicUIBuild::AddO(Blow, CoreKitBasicUIBuild::Box(Tree, Solid(Tree, CoreKitBasicUIColor::Orange), 4.f, 0.f), HAlign_Left, VAlign_Fill);
	UHorizontalBox* BlowRow = CoreKitBasicUIBuild::Row(Tree);
	CoreKitBasicUIBuild::AddO(Blow, BlowRow, HAlign_Fill, VAlign_Fill, FMargin(26.f, 16.f, 22.f, 16.f));
	UOverlay* Ring = CoreKitBasicUIBuild::Overlay(Tree);
	CoreKitBasicUIBuild::AddO(Ring, CoreKitBasicUIBuild::Icon(Tree, TEXT("T_UI_Ring4"), CoreKitBasicUIColor::Orange, 44.f));
	CoreKitBasicUIBuild::AddO(Ring, CoreKitBasicUIBuild::Icon(Tree, TEXT("T_UI_Circle"), CoreKitBasicUIColor::Orange, 14.f), HAlign_Center, VAlign_Center);
	CoreKitBasicUIBuild::AddH(BlowRow, Ring, FMargin(0.f, 0.f, 16.f, 0.f));
	UVerticalBox* BlowText = CoreKitBasicUIBuild::Column(Tree);
	FinalBlowTitle = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Orange, 0.18f));
	CoreKitBasicUIBuild::AddV(BlowText, FinalBlowTitle);
	FinalBlowTip = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(20.f), 400, CoreKitBasicUIColor::TextDim));
	FinalBlowTip->SetAutoWrapText(true);
	CoreKitBasicUIBuild::AddV(BlowText, FinalBlowTip, FMargin(0.f, 4.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddH(BlowRow, CoreKitBasicUIBuild::Box(Tree, BlowText, 470.f, 0.f), FMargin(0.f), VAlign_Center);
	CoreKitBasicUIBuild::AddH(Bottom, Blow, FMargin(0.f), VAlign_Bottom);
	FinalBlow = Blow;
	AddRise(Bottom, 1.35f);

	MakeButtons(COREKIT_BASICUI_TEXT("Retry", "RETRY"), COREKIT_BASICUI_TEXT("EnterNewRun", "ENTER · NEW RUN"));
}

void UCoreKitBasicUIDefeatScreen::ApplySummary(const UCoreKitBasicUIRunSummaryViewModel& Summary)
{
	UWidgetTree* Tree = WidgetTree;
	const float Death = Summary.GetDeathSeconds() > 0.f ? Summary.GetDeathSeconds() : Summary.GetTotalSeconds();
	const TArray<float> Bounds = StageBounds();
	const float End = FMath::Max(1.f, Bounds.Last());
	const float ShortBy = Summary.GetShortBySeconds() > 0.f ? Summary.GetShortBySeconds() : FMath::Max(0.f, End - Death);
	int32 Stage = Summary.GetDeathStage();
	if (Stage <= 0)
	{
		Stage = 1;
		for (int32 Index = 1; Index < Bounds.Num() - 1; ++Index)
		{
			Stage += Death >= Bounds[Index] ? 1 : 0;
		}
	}

	ResetCounts();
	AddCount(SurvivedStat, 1.1f, 0.6f, Death, true, false);
	AddCount(ShortStat, 1.17f, 0.6f, ShortBy, true, false);
	AddCount(KillsStat, 1.24f, 0.6f, static_cast<float>(Summary.GetKills()), false, false);
	AddCount(LevelStat, 1.31f, 0.6f, static_cast<float>(Summary.GetLevel()), false, true);

	DownLabel->SetText(FText::Format(COREKIT_BASICUI_TEXT("DownAt", "DOWN AT {0} · STAGE {1}"), FText::FromString(CoreKitBasicUIOverlay::Clock(Death)), FText::FromString(CoreKitBasicUIOverlay::TwoDigits(Stage))));

	// Bar: stage segments (gap 4) filled red up to the death point, glowing marker at the death point.
	TimelineBar->ClearChildren();
	const int32 Segments = Bounds.Num() - 1;
	const float Usable = TimelineWidth - 4.f * (Segments - 1);
	float X = 0.f;
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float Width = Usable * (Bounds[Index + 1] - Bounds[Index]) / End;
		CoreKitBasicUIBuild::Place(TimelineBar, Solid(Tree, CoreKitBasicUIColor::Divider), FAnchors(0.f, 0.f), FVector2D(X, 0.f), FVector2D(Width, 14.f));
		const float Fill = FMath::Clamp((Death - Bounds[Index]) / FMath::Max(1.f, Bounds[Index + 1] - Bounds[Index]), 0.f, 1.f);
		if (Fill > 0.f)
		{
			CoreKitBasicUIBuild::Place(TimelineBar, Solid(Tree, CoreKitBasicUIColor::Red), FAnchors(0.f, 0.f), FVector2D(X, 0.f), FVector2D(Width * Fill, 14.f));
		}
		X += Width + 4.f;
	}
	const float MarkerX = FMath::Clamp(Death / End, 0.f, 1.f) * TimelineWidth;
	CoreKitBasicUIBuild::Place(TimelineBar, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.7f))), FAnchors(0.f, 0.f), FVector2D(MarkerX - 18.f, -24.f), FVector2D(40.f, 62.f), FVector2D::ZeroVector, 1);
	CoreKitBasicUIBuild::Place(TimelineBar, Solid(Tree, CoreKitBasicUIColor::Red), FAnchors(0.f, 0.f), FVector2D(MarkerX - 2.f, -12.f), FVector2D(4.f, 38.f), FVector2D::ZeroVector, 2);

	BuildHeader->SetText(FText::Format(COREKIT_BASICUI_TEXT("BuildAtDeath", "BUILD AT DEATH · {0} {0}|plural(one=CARD,other=CARDS)"), FText::AsNumber(Summary.GetTotalCopies())));
	FillCards(BuildRow, Summary.GetFinalCards(), false, 8.f);

	const FText Killer = Summary.GetKillerType();
	const FText Tip = Summary.GetTip().IsEmpty() ? DefaultTip(Killer) : Summary.GetTip();
	FinalBlow->SetVisibility(Killer.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	FinalBlowTitle->SetText(FText::Format(COREKIT_BASICUI_TEXT("FinalBlow", "FINAL BLOW · {0}"), Killer.ToUpper()));
	FinalBlowTip->SetText(Tip);
	FinalBlowTip->SetVisibility(Tip.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UCoreKitBasicUIDefeatScreen::TickSequence(float InClock, float DeltaSeconds)
{
	enum : uint8 { FireGlitch = 1, FireSlow = 2, FireStamp = 4 };
	const bool bReduced = CoreKitBasicUIMotion::IsReducedMotion();
	UCoreKitBasicUIScreenEffectsComponent* Effects = UCoreKitBasicUIScreenEffectsComponent::Get(this);

	// 0-150 ms: glitch (RGB split / shake in the post-process; capped at 150 ms by the spec).
	if (!(Fired & FireGlitch))
	{
		Fired |= FireGlitch;
		PlayCue(TEXT("End.Glitch"));
		if (Effects && !bReduced)
		{
			Effects->PlayGlitch(0.15f);
		}
	}
	// 150-550 ms: slow motion + greyscale, then the terminal pause.
	if (!(Fired & FireSlow) && InClock >= 0.15f)
	{
		Fired |= FireSlow;
		PlayCue(TEXT("End.DefeatFlatline"));
		if (Effects)
		{
			Effects->SetSlowMotion(0.3f);
			bSlowMotion = true;
		}
	}
	if (Effects)
	{
		Effects->SetDeathAmount(FMath::Clamp((InClock - 0.15f) / 0.4f, 0.f, 1.f));
		if (bSlowMotion && InClock >= 0.55f)
		{
			Effects->SetSlowMotion(1.f);
			bSlowMotion = false;
		}
	}

	// Glitch bands jitter during the glitch window.
	for (int32 Index = 0; Index < Bands.Num(); ++Index)
	{
		const bool bGlitching = InClock < 0.15f && !bReduced;
		CoreKitBasicUIBuild::SetOffset(Bands[Index], FVector2D(bGlitching ? FMath::FRandRange(-12.f, 12.f) : 0.f, 0.f));
	}
	if (Vignette)
	{
		Vignette->SetRenderOpacity(InClock < 0.15f ? 1.f : FMath::Max(0.f, 1.f - (InClock - 0.15f) / 0.7f));
	}
	if (Background)
	{
		Background->SetRenderOpacity(0.95f * CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, FMath::Clamp((InClock - 0.4f) / 0.4f, 0.f, 1.f)));
	}

	// 550 ms: DEAD. stamps 1.7 -> 1 (SLAM); ghosts enter magenta/white at +-14 px (420 ms) and settle white/red at +-8 px.
	if (!(Fired & FireStamp) && InClock >= 0.55f)
	{
		Fired |= FireStamp;
		PlayCue(TEXT("End.DefeatJingle"));
	}
	const float StampA = FMath::Clamp((InClock - 0.55f) / 0.38f, 0.f, 1.f);
	if (Title)
	{
		const float Scale = bReduced ? 1.f : FMath::Lerp(1.7f, 1.f, CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Slam, StampA));
		CoreKitBasicUIOverlay::Pose(Title, FVector2D::ZeroVector, Scale, 0.f, FMath::Clamp(StampA * 2.f, 0.f, 1.f));
	}
	const float GhostT = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, FMath::Clamp((InClock - 0.56f) / 0.42f, 0.f, 1.f));
	const bool bGhostsVisible = InClock >= 0.56f;
	if (GhostA && GhostB)
	{
		const FLinearColor ColorA = FMath::Lerp(CoreKitBasicUIColor::Magenta, CoreKitBasicUIColor::White, GhostT);
		const FLinearColor ColorB = FMath::Lerp(CoreKitBasicUIColor::White, CoreKitBasicUIColor::Red, GhostT);
		GhostA->SetColorAndOpacity(FSlateColor(ColorA));
		GhostB->SetColorAndOpacity(FSlateColor(ColorB));
		const float Spread = bReduced ? 8.f : FMath::Lerp(14.f, 8.f, GhostT);
		const float Opacity = bGhostsVisible ? FMath::Lerp(0.85f, 0.55f, GhostT) : 0.f;
		CoreKitBasicUIOverlay::Pose(GhostA, FVector2D(-Spread, 4.f), 1.f, 0.f, Opacity);
		CoreKitBasicUIOverlay::Pose(GhostB, FVector2D(Spread, -4.f), 1.f, 0.f, Opacity);
	}
}

void UCoreKitBasicUIDefeatScreen::NativeDestruct()
{
	// Never leave the world slowed or grey after Retry / Main Menu.
	if (UCoreKitBasicUIScreenEffectsComponent* Effects = UCoreKitBasicUIScreenEffectsComponent::Get(this))
	{
		Effects->SetSlowMotion(1.f);
		Effects->SetDeathAmount(0.f);
	}
	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
