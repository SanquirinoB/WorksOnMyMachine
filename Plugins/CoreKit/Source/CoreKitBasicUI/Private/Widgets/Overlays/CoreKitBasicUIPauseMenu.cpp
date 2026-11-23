#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIPauseMenu.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitBasicUI/Effects/CoreKitBasicUIScreenEffectsComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUIViewModels.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/FrontEnd/CoreKitBasicUIFrontEndMenus.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIConfirmPrompt.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIPauseMenu"

namespace
{
	enum EPauseEntry : int32
	{
		EntryResume,
		EntryHowToPlay,
		EntrySettings,
		EntryRestart,
		EntryQuit
	};

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, const CoreKitBasicUIBuild::FTextStyle& Style, FName Name = NAME_None)
	{
		UTextBlock* Block = CoreKitBasicUIBuild::Text(Tree, Text, Style, Name);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	}

	/** "RUN STATUS ———" section header: mono label followed by a Seam rule. */
	UHorizontalBox* SectionHeader(UWidgetTree* Tree, const FText& Text, UTextBlock** OutLabel = nullptr)
	{
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(Tree);
		UTextBlock* Text0 = Label(Tree, Text, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.22f));
		CoreKitBasicUIBuild::AddH(Row, Text0, FMargin(0.f, 0.f, 12.f, 0.f));
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(Tree, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Seam)), 0.f, 1.f), FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
		if (OutLabel)
		{
			*OutLabel = Text0;
		}
		return Row;
	}

	void SetBar(UImage* Fill, float Fraction)
	{
		UHorizontalBox* Bar = Fill ? Cast<UHorizontalBox>(Fill->GetParent()) : nullptr;
		if (!Bar || Bar->GetChildrenCount() < 2)
		{
			return;
		}
		const float F = FMath::Clamp(Fraction, 0.f, 1.f);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (UHorizontalBoxSlot* BarSlot = Cast<UHorizontalBoxSlot>(Bar->GetChildAt(Index)->Slot))
			{
				FSlateChildSize Size(ESlateSizeRule::Fill);
				Size.Value = Index == 0 ? F : 1.f - F;
				BarSlot->SetSize(Size);
			}
		}
	}
}

UCoreKitBasicUIPauseMenu::UCoreKitBasicUIPauseMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowReturn = true;
	bAllowSections = false;
	ActivationInputCooldown = 0.15f;
}

UWidget* UCoreKitBasicUIPauseMenu::MakeTile(const FText& LabelText, TObjectPtr<UCoreKitBasicUITabularNumber>& OutValue, TObjectPtr<UTextBlock>* OutSub, FLinearColor ValueColor, bool bBar)
{
	UWidgetTree* Tree = WidgetTree;
	UBorder* Tile = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Hull), FMargin(20.f, 16.f));
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(Tree);
	Tile->SetContent(Column);
	CoreKitBasicUIBuild::AddV(Column, Label(Tree, LabelText, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Steel, 0.16f)));
	OutValue = Tree->ConstructWidget<UCoreKitBasicUITabularNumber>(UCoreKitBasicUITabularNumber::StaticClass());
	OutValue->Setup(CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(44.f), 600, ValueColor));
	CoreKitBasicUIBuild::AddV(Column, OutValue, FMargin(0.f, 4.f, 0.f, 0.f), HAlign_Left);
	if (bBar)
	{
		UHorizontalBox* Bar = CoreKitBasicUIBuild::Row(Tree);
		UImage* Fill = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta));
		CoreKitBasicUIBuild::AddH(Bar, Fill, FMargin(0.f), VAlign_Fill, HAlign_Fill, 0.01f);
		CoreKitBasicUIBuild::AddH(Bar, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Seam)), FMargin(0.f), VAlign_Fill, HAlign_Fill, 0.99f);
		CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(Tree, Bar, 0.f, 4.f), FMargin(0.f, 10.f, 0.f, 0.f));
		TimeBarFill = Fill;
	}
	else if (OutSub)
	{
		*OutSub = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Steel, 0.14f));
		CoreKitBasicUIBuild::AddV(Column, *OutSub, FMargin(0.f, 4.f, 0.f, 0.f));
	}
	return Tile;
}

void UCoreKitBasicUIPauseMenu::BuildTree()
{
	UWidgetTree* Tree = WidgetTree;
	UCanvasPanel* Root = CoreKitBasicUIBuild::Canvas(Tree, TEXT("PauseRoot"));
	Tree->RootWidget = Root;

	// World: blur 14 px, veil rgba(10,10,12,.6) + scanlines (the post-process brings brightness/saturation down).
	Blur = Tree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("WorldBlur"));
	Blur->SetBlurStrength(0.f);
	CoreKitBasicUIBuild::Fill(Root, Blur, FMargin(0.f), 0);
	UCanvasPanel* VeilGroup = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Veil"));
	VeilGroup->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoreKitBasicUIBuild::Fill(VeilGroup, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.6f))));
	CoreKitBasicUIBuild::Fill(VeilGroup, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Scanlines(0.03f)));
	CoreKitBasicUIBuild::Fill(Root, VeilGroup, FMargin(0.f), 1);
	Veil = VeilGroup;

	// Left column: title + entries.
	constexpr float EntryWidth = 520.f;
	UVerticalBox* Left = CoreKitBasicUIBuild::Column(Tree, TEXT("LeftColumn"));
	CoreKitBasicUIBuild::Place(Root, CoreKitBasicUIBuild::Box(Tree, Left, 560.f, 0.f), FAnchors(0.f, 0.f), FVector2D(120.f, 110.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 5);
	UVerticalBox* Title = CoreKitBasicUIBuild::Column(Tree, TEXT("Title"));
	CoreKitBasicUIBuild::AddV(Title, Label(Tree, COREKIT_BASICUI_TEXT("Halted", "SIMULATION HALTED"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), 500, CoreKitBasicUIColor::Magenta, 0.24f)));
	// CSS line-height .85 for the 120 px title: trim Slate's taller line box.
	const float PausedTrim = CoreKitBasicUIOverlay::CssLineOffset(UCoreKitBasicUIStyle::Font(ECoreKitBasicUIFontFamily::Display, 700, CoreKitBasicUIBuild::Px(120.f)), 120.f * 0.85f);
	CoreKitBasicUIBuild::AddV(Title, Label(Tree, COREKIT_BASICUI_TEXT("Paused", "PAUSED"), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(120.f), 700)), FMargin(0.f, 6.f + PausedTrim, 0.f, PausedTrim));
	CoreKitBasicUIBuild::AddV(Left, Title);
	TitleGroup = Title;

	struct FEntry { FText Label; FText Sub; };
	const FEntry EntryTexts[] = {
		{ COREKIT_BASICUI_TEXT("Resume", "RESUME"), COREKIT_BASICUI_TEXT("ResumeKey", "ESC") },
		{ COREKIT_BASICUI_TEXT("HowToPlay", "HOW TO PLAY"), COREKIT_BASICUI_TEXT("HowToPlaySub", "CONTROLS") },
		{ COREKIT_BASICUI_TEXT("Settings", "SETTINGS"), COREKIT_BASICUI_TEXT("SettingsSub", "INPUT · AUDIO · VIDEO") },
		{ COREKIT_BASICUI_TEXT("Restart", "RESTART RUN"), COREKIT_BASICUI_TEXT("RestartSub", "START OVER") },
		{ COREKIT_BASICUI_TEXT("Quit", "QUIT TO MENU"), COREKIT_BASICUI_TEXT("QuitSub", "RUN IS LOST") } };
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(EntryTexts)); ++Index)
	{
		UCoreKitBasicUIActionButton* Button = Tree->ConstructWidget<UCoreKitBasicUIActionButton>(UCoreKitBasicUIActionButton::StaticClass(), *FString::Printf(TEXT("Entry%d"), Index));
		const bool bResume = Index == EntryResume;
		Button->Setup(EntryTexts[Index].Label, EntryTexts[Index].Sub, bResume ? ECoreKitBasicUIButtonVariant::Primary : ECoreKitBasicUIButtonVariant::MenuList,
			bResume ? 74.f : 66.f, bResume ? 34.f : 30.f, bResume);
		Button->Index = Index;
		Button->OnActivated.AddUObject(this, &ThisClass::HandleEntry);
		// Left-aligned stack, one common width (520 px, as the Main Menu nav); RESUME 74 px, the others 66 px.
		CoreKitBasicUIBuild::AddV(Left, CoreKitBasicUIBuild::Box(Tree, Button, EntryWidth, 0.f), FMargin(0.f, Index == 0 ? 30.f : 12.f, 0.f, 0.f), HAlign_Left);
		Entries.Add(Button);
	}

	// Right section: run status tiles + build list.
	UVerticalBox* Right = CoreKitBasicUIBuild::Column(Tree, TEXT("RunStatus"));
	CoreKitBasicUIBuild::Place(Root, CoreKitBasicUIBuild::Box(Tree, Right, 860.f, 0.f), FAnchors(1.f, 0.f), FVector2D(-120.f, 110.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f), 5);
	StatusGroup = Right;
	CoreKitBasicUIBuild::AddV(Right, SectionHeader(Tree, COREKIT_BASICUI_TEXT("RunStatusHeader", "RUN STATUS")));
	UHorizontalBox* Tiles = CoreKitBasicUIBuild::Row(Tree, TEXT("Tiles"));
	CoreKitBasicUIBuild::AddV(Right, Tiles, FMargin(0.f, 22.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddH(Tiles, MakeTile(COREKIT_BASICUI_TEXT("Time", "TIME"), TimeValue, nullptr, CoreKitBasicUIColor::White, true), FMargin(0.f), VAlign_Fill, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(Tiles, MakeTile(COREKIT_BASICUI_TEXT("Stage", "STAGE"), StageValue, &StageSub, CoreKitBasicUIColor::White, false), FMargin(12.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(Tiles, MakeTile(COREKIT_BASICUI_TEXT("Level", "LEVEL"), LevelValue, &LevelSub, CoreKitBasicUIColor::Lime, false), FMargin(12.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(Tiles, MakeTile(COREKIT_BASICUI_TEXT("Kills", "KILLS"), KillsValue, &KillsSub, CoreKitBasicUIColor::White, false), FMargin(12.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);

	UTextBlock* BuildHeaderText = nullptr;
	UHorizontalBox* BuildHeaderRow = SectionHeader(Tree, FText::GetEmpty(), &BuildHeaderText);
	BuildHeader = BuildHeaderText;
	CoreKitBasicUIBuild::AddV(Right, BuildHeaderRow, FMargin(0.f, 32.f, 0.f, 0.f));
	UBorder* BuildPanel = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Chamfer(16, CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Hull, 0.85f)), FMargin(26.f, 6.f), TEXT("BuildPanel"));
	UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BuildScroll"));
	BuildList = CoreKitBasicUIBuild::Column(Tree, TEXT("BuildList"));
	Scroll->AddChild(BuildList);
	USizeBox* ScrollBox = CoreKitBasicUIBuild::Box(Tree, Scroll, 0.f, 0.f);
	ScrollBox->SetMaxDesiredHeight(560.f);
	BuildPanel->SetContent(ScrollBox);
	CoreKitBasicUIBuild::AddV(Right, BuildPanel, FMargin(0.f, 22.f, 0.f, 0.f));

	// Footer: navigation glyphs · "TIMER, SPAWNS AND ENEMIES FROZEN".
	UVerticalBox* FooterBox = CoreKitBasicUIBuild::Column(Tree, TEXT("Footer"));
	UCanvasPanelSlot* FooterSlot = Root->AddChildToCanvas(FooterBox);
	FooterSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	FooterSlot->SetAlignment(FVector2D(0.f, 1.f));
	FooterSlot->SetOffsets(FMargin(120.f, -44.f, 120.f, 60.f));
	FooterSlot->SetZOrder(5);
	CoreKitBasicUIBuild::AddV(FooterBox, CoreKitBasicUIBuild::Box(Tree, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Plate)), 0.f, 1.f));
	UHorizontalBox* FooterRow = CoreKitBasicUIBuild::Row(Tree);
	CoreKitBasicUIBuild::AddV(FooterBox, FooterRow, FMargin(0.f, 18.f, 0.f, 0.f));
	const CoreKitBasicUIBuild::FTextStyle FooterStyle = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.14f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UCoreKitBasicUIKeycap* Key = Tree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass());
		CoreKitBasicUIBuild::AddH(FooterRow, Key, FMargin(Index == 2 ? 28.f : 0.f, 0.f, 10.f, 0.f));
		FooterKeys.Add(Key);
		if (Index == 1)
		{
			CoreKitBasicUIBuild::AddH(FooterRow, Label(Tree, COREKIT_BASICUI_TEXT("Navigate", "NAVIGATE"), FooterStyle));
		}
	}
	CoreKitBasicUIBuild::AddH(FooterRow, Label(Tree, COREKIT_BASICUI_TEXT("Select", "SELECT"), FooterStyle));
	CoreKitBasicUIBuild::AddH(FooterRow, Label(Tree, COREKIT_BASICUI_TEXT("Frozen", "TIMER, SPAWNS AND ENEMIES FROZEN"), FooterStyle), FMargin(0.f), VAlign_Center, HAlign_Right, 1.f);
	FooterGroup = FooterBox;
	RefreshFooterKeys();
}

void UCoreKitBasicUIPauseMenu::NativeConstruct()
{
	Super::NativeConstruct();
	Clock = 0.f;
	bClosing = false;
	Enter.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
	Leave.Snap(1.f);
	PlayCue(TEXT("UI.Open"));
	if (!bLowPass)
	{
		CoreKitBasicUIOverlay::PushLowPass(this);
		bLowPass = true;
	}
	RefreshStatus();
	RefreshBuild();
	for (UCoreKitBasicUIActionButton* Entry : Entries)
	{
		CoreKitBasicUIOverlay::FillButtonContent(Entry);
	}
}

void UCoreKitBasicUIPauseMenu::NativeDestruct()
{
	if (bLowPass)
	{
		CoreKitBasicUIOverlay::PopLowPass(this);
		bLowPass = false;
	}
	if (UCoreKitBasicUIScreenEffectsComponent* Effects = UCoreKitBasicUIScreenEffectsComponent::Get(this))
	{
		Effects->SetFreezeAmount(0.f);
	}
	Super::NativeDestruct();
}

UWidget* UCoreKitBasicUIPauseMenu::GetInitialFocus_Implementation() const
{
	return Entries.Num() > 0 && Entries[0] ? Entries[0]->GetButton() : nullptr;
}

bool UCoreKitBasicUIPauseMenu::HandleReturn_Implementation()
{
	Resume();
	return true;
}

void UCoreKitBasicUIPauseMenu::Resume()
{
	if (bClosing)
	{
		return;
	}
	UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	UCoreKitBasicUIRunViewModel* Run = Context ? Context->GetRun() : nullptr;
	if (Run && Run->TogglePause.IsBound())
	{
		Run->RequestTogglePause();
	}
	else
	{
		BeginExit();
	}
}

void UCoreKitBasicUIPauseMenu::BeginExit()
{
	if (bClosing)
	{
		return;
	}
	bClosing = true;
	Leave.Start(1.f, 0.f, ECoreKitBasicUIMotion::Exit, 0.14f);
	PlayCue(TEXT("UI.Close"));
	if (bLowPass)
	{
		CoreKitBasicUIOverlay::PopLowPass(this);
		bLowPass = false;
	}
}

void UCoreKitBasicUIPauseMenu::HandleEntry(UCoreKitBasicUIActionButton* Button)
{
	if (!Button || bClosing)
	{
		return;
	}
	APlayerController* Controller = GetOwningPlayer();
	UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	UCoreKitBasicUIRunViewModel* Run = Context ? Context->GetRun() : nullptr;
	const float Seconds = Run ? Run->GetElapsedSeconds() : 0.f;
	const int32 Level = Context && Context->GetProgression() ? Context->GetProgression()->GetLevel() : 1;
	const int32 Cards = Context && Context->GetBuild() ? Context->GetBuild()->GetTotalCopies() : 0;
	TWeakObjectPtr<UCoreKitBasicUIRunViewModel> WeakRun = Run;
	switch (Button->Index)
	{
	case EntryResume:
		Resume();
		break;
	case EntryHowToPlay:
		CoreKitBasicUIFrontEnd::OpenHowToPlay(Controller);
		break;
	case EntrySettings:
		CoreKitBasicUIFrontEnd::OpenSettings(Controller);
		break;
	case EntryRestart:
		CoreKitBasicUIPrompt::ShowPreset(Controller, ECoreKitBasicUIPromptPreset::RestartRun, [WeakRun](bool bConfirmed)
		{
			if (bConfirmed && WeakRun.IsValid())
			{
				WeakRun->RequestRestart();
			}
		}, Seconds, Level, Cards);
		break;
	case EntryQuit:
		CoreKitBasicUIPrompt::ShowPreset(Controller, ECoreKitBasicUIPromptPreset::QuitToMenu, [WeakRun](bool bConfirmed)
		{
			if (bConfirmed && WeakRun.IsValid())
			{
				WeakRun->RequestQuitToMenu();
			}
		}, Seconds, Level, Cards);
		break;
	default:
		break;
	}
}

void UCoreKitBasicUIPauseMenu::RefreshStatus()
{
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	if (!Context)
	{
		return;
	}
	if (const UCoreKitBasicUIRunViewModel* Run = Context->GetRun())
	{
		const float Elapsed = Run->GetElapsedSeconds();
		if (TimeValue)
		{
			TimeValue->SetValue(CoreKitBasicUIOverlay::Clock(Elapsed));
		}
		SetBar(TimeBarFill, Run->GetSurvivalSeconds() > 0.f ? Elapsed / Run->GetSurvivalSeconds() : 0.f);
		if (StageValue)
		{
			StageValue->SetValue(CoreKitBasicUIOverlay::TwoDigits(Run->GetStageIndex() + 1));
		}
		if (StageSub)
		{
			const FText Name = Run->GetStageName().IsEmpty() ? CoreKitBasicUIOverlay::StageName(Run->GetStageIndex()) : Run->GetStageName();
			StageSub->SetText(Name.ToUpper());
			StageSub->SetColorAndOpacity(FSlateColor(CoreKitBasicUIColor::Magenta));
		}
		if (KillsValue)
		{
			KillsValue->SetValue(FString::FromInt(Run->GetKills()));
		}
	}
	if (const UCoreKitBasicUIProgressionViewModel* Progression = Context->GetProgression())
	{
		if (LevelValue)
		{
			LevelValue->SetValue(CoreKitBasicUIOverlay::TwoDigits(Progression->GetLevel()));
		}
		if (LevelSub)
		{
			LevelSub->SetText(FText::Format(COREKIT_BASICUI_TEXT("ToNext", "{0}% TO NEXT"), FText::AsNumber(FMath::RoundToInt(FMath::Clamp(Progression->GetXPFraction(), 0.f, 1.f) * 100.f))));
		}
	}
	if (const UCoreKitBasicUIVitalsViewModel* Vitals = Context->GetVitals(); Vitals && KillsSub)
	{
		KillsSub->SetText(FText::Format(COREKIT_BASICUI_TEXT("Hull", "HULL {0} / {1}"), FText::AsNumber(FMath::CeilToInt(Vitals->GetHealth())), FText::AsNumber(FMath::RoundToInt(Vitals->GetMaxHealth()))));
		KillsSub->SetColorAndOpacity(FSlateColor(CoreKitBasicUIColor::Mint));
	}
}

void UCoreKitBasicUIPauseMenu::RefreshBuild()
{
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	const UCoreKitBasicUIBuildViewModel* Build = Context ? Context->GetBuild() : nullptr;
	if (!Build || !BuildList)
	{
		return;
	}
	BuildSerial = Build->GetPickSerial();
	if (BuildHeader)
	{
		BuildHeader->SetText(FText::Format(COREKIT_BASICUI_TEXT("BuildHeader", "YOUR BUILD · {0} {0}|plural(one=CARD,other=CARDS)"), FText::AsNumber(Build->GetTotalCopies())));
	}
	BuildList->ClearChildren();
	UWidgetTree* Tree = WidgetTree;
	const TArray<FCoreKitBasicUICardData>& Cards = Build->GetCards();
	if (Cards.IsEmpty())
	{
		CoreKitBasicUIBuild::AddV(BuildList, Label(Tree, COREKIT_BASICUI_TEXT("NoCards", "NO CARDS YET · LEVEL UP TO DRAW"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.14f)), FMargin(0.f, 14.f));
		return;
	}
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FCoreKitBasicUICardData& Card = Cards[Index];
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(Tree);
		UCoreKitBasicUICardWidget* Thumb = Tree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass());
		Thumb->SetVariant(ECoreKitBasicUICardVariant::Thumb, 34.f);
		Thumb->SetCard(Card);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(Tree, Thumb, 44.f, 0.f), FMargin(0.f, 0.f, 16.f, 0.f), VAlign_Center, HAlign_Left);
		UHorizontalBox* NameRow = CoreKitBasicUIBuild::Row(Tree);
		CoreKitBasicUIBuild::AddH(NameRow, Label(Tree, Card.Name.ToUpper(), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(24.f), 700)), FMargin(0.f), VAlign_Center);
		if (Card.bSpecial)
		{
			UBorder* Chip = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta), FMargin(8.f, 3.f));
			Chip->SetContent(Label(Tree, COREKIT_BASICUI_TEXT("Special", "SPECIAL"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(11.f), 700, CoreKitBasicUIColor::Soot, 0.14f)));
			CoreKitBasicUIBuild::AddH(NameRow, Chip, FMargin(12.f, 0.f, 0.f, 0.f), VAlign_Center);
		}
		else
		{
			CoreKitBasicUIBuild::AddH(NameRow, Label(Tree, FText::FromString(FString::Printf(TEXT("×%d"), FMath::Max(1, Card.Copies))),
				CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), 500, CoreKitBasicUIColor::Steel, 0.f)), FMargin(8.f, 0.f, 0.f, 0.f), VAlign_Center);
		}
		CoreKitBasicUIBuild::AddH(Row, NameRow, FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
		CoreKitBasicUIBuild::AddH(Row, Label(Tree, Card.SummedEffect, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(24.f), 600, UCoreKitBasicUIStyle::SuitColor(Card.Suit))), FMargin(16.f, 0.f, 0.f, 0.f), VAlign_Center, HAlign_Right);
		CoreKitBasicUIBuild::AddV(BuildList, Row, FMargin(0.f, 12.f));
		if (Index < Cards.Num() - 1)
		{
			CoreKitBasicUIBuild::AddV(BuildList, CoreKitBasicUIBuild::Box(Tree, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Plate)), 0.f, 1.f));
		}
	}
}

void UCoreKitBasicUIPauseMenu::RefreshFooterKeys()
{
	const ULocalPlayer* Player = GetOwningLocalPlayer();
	const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	const bool bPad = Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad;
	if (bKeysSet && bPad == bPadKeys)
	{
		return;
	}
	bKeysSet = true;
	bPadKeys = bPad;
	const FKey Keys[3] = {
		bPad ? EKeys::Gamepad_DPad_Up : EKeys::W,
		bPad ? EKeys::Gamepad_DPad_Down : EKeys::S,
		bPad ? EKeys::Gamepad_FaceButton_Bottom : EKeys::Enter };
	for (int32 Index = 0; Index < FooterKeys.Num() && Index < 3; ++Index)
	{
		FooterKeys[Index]->SetKey(Keys[Index]);
	}
}

void UCoreKitBasicUIPauseMenu::TickMotion(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	Enter.Tick(DeltaSeconds);
	Leave.Tick(DeltaSeconds);
	const float Out = Leave.Value;
	const float A = Enter.Value * Out;
	const bool bReduced = CoreKitBasicUIMotion::IsReducedMotion();

	if (Blur)
	{
		Blur->SetBlurStrength(14.f * A);
	}
	if (Veil)
	{
		Veil->SetRenderOpacity(A);
	}
	if (UCoreKitBasicUIScreenEffectsComponent* Effects = UCoreKitBasicUIScreenEffectsComponent::Get(this))
	{
		Effects->SetFreezeAmount(A);
	}
	CoreKitBasicUIOverlay::Pose(TitleGroup, FVector2D(0.f, bReduced ? 0.f : -12.f * (1.f - Enter.Value)), 1.f, 0.f, A);
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const float Local = FMath::Clamp((Clock - 0.12f - Index * 0.05f) / 0.2f, 0.f, 1.f);
		const float E = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, Local);
		CoreKitBasicUIOverlay::Pose(Entries[Index], FVector2D(bReduced ? 0.f : -24.f * (1.f - E), 0.f), 1.f, 0.f, E * Out);
	}
	const float Side = CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Settle, FMath::Clamp((Clock - 0.12f) / 0.2f, 0.f, 1.f));
	CoreKitBasicUIOverlay::Pose(StatusGroup, FVector2D::ZeroVector, 1.f, 0.f, Side * Out);
	CoreKitBasicUIOverlay::Pose(FooterGroup, FVector2D::ZeroVector, 1.f, 0.f, Side * Out);

	RefreshTimer -= DeltaSeconds;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.25f;
		RefreshStatus();
		RefreshFooterKeys();
		const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
		if (Context && Context->GetBuild() && Context->GetBuild()->GetPickSerial() != BuildSerial)
		{
			RefreshBuild();
		}
	}

	if (bClosing && !Leave.IsActive())
	{
		bClosing = false;
		Leave.Snap(0.f);
		CloseMenu();
	}
}

#undef LOCTEXT_NAMESPACE
