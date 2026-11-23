#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "Widgets/FrontEnd/CoreKitBasicUIFrontEndCommon.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitNavigation/CoreKitButton.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUICredits"

namespace
{
	struct FCreditLine
	{
		FText Left;
		FText Right;
	};

	struct FFontCredit
	{
		const TCHAR* Family;
		const TCHAR* Copyright;
	};

	/** Copyright lines as in each family's OFL.txt (ArtSource/ThirdParty/Fonts). */
	const FFontCredit Fonts[] = {
		{ TEXT("Saira Condensed"), TEXT("© 2016 The Saira Project Authors") },
		{ TEXT("Saira Stencil One"), TEXT("© 2019 The Saira Stencil Project Authors") },
		{ TEXT("Barlow Semi Condensed"), TEXT("© 2017 The Barlow Project Authors") },
		{ TEXT("JetBrains Mono"), TEXT("© 2020 The JetBrains Mono Project Authors") } };

	/** Kenney packs of ArtSource/ThirdParty/Manifest.json, with their display names. */
	struct FKenneyPack
	{
		const TCHAR* Id;
		const TCHAR* Name;
	};
	const FKenneyPack Packs[] = {
		{ TEXT("InputPrompts"), TEXT("Input Prompts 1.5") },
		{ TEXT("InterfaceSounds"), TEXT("Interface Sounds") },
		{ TEXT("UIAudio"), TEXT("UI Audio") },
		{ TEXT("DigitalAudio"), TEXT("Digital Audio") },
		{ TEXT("ImpactSounds"), TEXT("Impact Sounds") },
		{ TEXT("SciFiSounds"), TEXT("Sci-Fi Sounds") },
		{ TEXT("CasinoAudio"), TEXT("Casino Audio") },
		{ TEXT("MusicJingles"), TEXT("Music Jingles") } };
}

UCoreKitBasicUICreditsMenu::UCoreKitBasicUICreditsMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowSections = false;
	SetIsFocusable(true);
}

FText UCoreKitBasicUICreditsMenu::GetTeamName()
{
	return GetDefault<UCoreKitBasicUISettings>()->TeamName;
}

TArray<FString> UCoreKitBasicUICreditsMenu::GetKenneyPacks()
{
	TArray<FString> Ids;
	for (const FKenneyPack& Pack : Packs)
	{
		Ids.Add(Pack.Id);
	}
	return Ids;
}

TArray<FString> UCoreKitBasicUICreditsMenu::GetFontFamilies()
{
	TArray<FString> Families;
	for (const FFontCredit& Font : Fonts)
	{
		Families.Add(Font.Family);
	}
	return Families;
}

void UCoreKitBasicUICreditsMenu::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIFrontEndUI::MakeScreenRoot(WidgetTree);

	UHorizontalBox* Header = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Header"));
	CoreKitBasicUIBuild::AddH(Header, CoreKitBasicUIFrontEndUI::MakeTitleBlock(WidgetTree, COREKIT_BASICUI_TEXT("Kicker", "PERSONNEL FILE"), COREKIT_BASICUI_TEXT("Title", "Credits")), FMargin(0.f), VAlign_Bottom, HAlign_Left, 1.f);
	UCoreKitButton* Back = nullptr;
	CoreKitBasicUIBuild::AddH(Header, CoreKitBasicUIFrontEndUI::MakeKeyHint(WidgetTree, EKeys::Escape, EKeys::Gamepad_FaceButton_Right, COREKIT_BASICUI_TEXT("Back", "BACK"), CoreKitBasicUIColor::TextDim, CoreKitBasicUIColor::Steel, &Back), FMargin(0.f, 0.f, 0.f, 8.f), VAlign_Bottom);
	BackButton = Back;
	Back->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleBack);
	UCanvasPanelSlot* HeaderSlot = CoreKitBasicUIBuild::Place(Root, Header, FAnchors(0.f, 0.f, 1.f, 0.f), FVector2D::ZeroVector);
	HeaderSlot->SetOffsets(FMargin(120.f, 64.f, 120.f, 0.f));
	HeaderSlot->SetAutoSize(true);

	const CoreKitBasicUIBuild::FTextStyle RowLabel = CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(22.f), 600, CoreKitBasicUIColor::White);
	const CoreKitBasicUIBuild::FTextStyle RowValue = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.14f);
	const CoreKitBasicUIBuild::FTextStyle Small = CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(18.f), 400, CoreKitBasicUIColor::TextDim);
	auto Line = [this, &RowLabel, &RowValue](UVerticalBox* Column, const FText& Left, const FText& Right, bool bLast)
	{
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, Left, RowLabel), FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, Right, RowValue), FMargin(16.f, 0.f, 0.f, 0.f), VAlign_Center, HAlign_Right);
		CoreKitBasicUIBuild::AddV(Column, Row, FMargin(0.f, 11.f));
		if (!bLast)
		{
			CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIFrontEndUI::MakeRule(WidgetTree, CoreKitBasicUIColor::Plate));
		}
	};

	// Column 1: the team. Only the author name (user request), in the display style.
	UVerticalBox* TeamColumn = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Team"));
	CoreKitBasicUIBuild::AddV(TeamColumn, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("JamTeam", "Team")), FMargin(0.f, 0.f, 0.f, 14.f));
	CoreKitBasicUIBuild::AddV(TeamColumn, CoreKitBasicUIBuild::Text(WidgetTree, GetTeamName(), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(56.f), 800)));

	// Column 2: third-party attributions (Kenney CC0 packs; OFL fonts).
	UVerticalBox* ThirdColumn = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("ThirdParty"));
	CoreKitBasicUIBuild::AddV(ThirdColumn, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("ThirdParty", "Third-party")), FMargin(0.f, 0.f, 0.f, 14.f));
	{
		UBorder* Plate = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(14, CoreKitBasicUIColor::Hull), FMargin(22.f, 18.f));
		UVerticalBox* Content = CoreKitBasicUIBuild::Column(WidgetTree);
		Plate->SetContent(Content);
		UHorizontalBox* Title = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddH(Title, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Kenney", "KENNEY"), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(28.f), 700)), FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
		CoreKitBasicUIBuild::AddH(Title, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("KenneyLicense", "CC0 1.0 · KENNEY.NL"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 700, CoreKitBasicUIColor::Magenta, 0.18f)), FMargin(0.f), VAlign_Center, HAlign_Right);
		CoreKitBasicUIBuild::AddV(Content, Title);
		FString PackList;
		for (const FKenneyPack& Pack : Packs)
		{
			PackList += PackList.IsEmpty() ? FString(Pack.Name) : FString(TEXT(" · ")) + Pack.Name;
		}
		CoreKitBasicUIBuild::AddV(Content, CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree, FText::FromString(PackList), CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(19.f), 500, CoreKitBasicUIColor::White)), FMargin(0.f, 8.f, 0.f, 0.f));
		CoreKitBasicUIBuild::AddV(Content, CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree,
			COREKIT_BASICUI_TEXT("KenneyNote", "Gamepad glyphs and interface sounds. Public domain: credit is not required, it is given with thanks."), Small), FMargin(0.f, 6.f, 0.f, 0.f));
		CoreKitBasicUIBuild::AddV(ThirdColumn, Plate);
	}
	{
		UBorder* Plate = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(14, CoreKitBasicUIColor::Hull), FMargin(22.f, 18.f));
		UVerticalBox* Content = CoreKitBasicUIBuild::Column(WidgetTree);
		Plate->SetContent(Content);
		UHorizontalBox* Title = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddH(Title, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Fonts", "FONTS"), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(28.f), 700)), FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
		CoreKitBasicUIBuild::AddH(Title, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("OFL", "SIL OPEN FONT LICENSE 1.1"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 700, CoreKitBasicUIColor::Magenta, 0.18f)), FMargin(0.f), VAlign_Center, HAlign_Right);
		CoreKitBasicUIBuild::AddV(Content, Title, FMargin(0.f, 0.f, 0.f, 4.f));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Fonts); ++Index)
		{
			UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
			CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(Fonts[Index].Family), CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(19.f), 600, CoreKitBasicUIColor::White)), FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
			CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(Fonts[Index].Copyright), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(11.f), 500, CoreKitBasicUIColor::Muted, 0.06f)), FMargin(12.f, 0.f, 0.f, 0.f), VAlign_Center, HAlign_Right);
			CoreKitBasicUIBuild::AddV(Content, Row, FMargin(0.f, 6.f, 0.f, 0.f));
		}
		CoreKitBasicUIBuild::AddV(Content, CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree,
			COREKIT_BASICUI_TEXT("OFLNote", "Font licence notices accompany the game."), Small), FMargin(0.f, 10.f, 0.f, 0.f));
		CoreKitBasicUIBuild::AddV(ThirdColumn, Plate, FMargin(0.f, 14.f, 0.f, 0.f));
	}

	// Column 3: tools.
	UVerticalBox* ToolsColumn = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Tools"));
	CoreKitBasicUIBuild::AddV(ToolsColumn, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("Tools", "Tools")), FMargin(0.f, 0.f, 0.f, 14.f));
	const FCreditLine Tools[] = {
		{ COREKIT_BASICUI_TEXT("ToolUE", "Unreal Engine 5.8"), COREKIT_BASICUI_TEXT("ToolUEBy", "EPIC GAMES") },
		{ COREKIT_BASICUI_TEXT("ToolInput", "Enhanced Input · UMG · Niagara"), COREKIT_BASICUI_TEXT("ToolInputBy", "UNREAL ENGINE") },
		{ COREKIT_BASICUI_TEXT("ToolCoreKit", "CoreKit"), COREKIT_BASICUI_TEXT("ToolCoreKitBy", "PROJECT PLUGINS") },
		{ COREKIT_BASICUI_TEXT("ToolParagon", "Paragon: Crunch animations"), COREKIT_BASICUI_TEXT("ToolParagonBy", "EPIC GAMES · UE EULA") },
		{ COREKIT_BASICUI_TEXT("ToolElevenLabs", "ElevenLabs: music"), COREKIT_BASICUI_TEXT("ToolElevenLabsBy", "ELEVENLABS") },
		{ COREKIT_BASICUI_TEXT("ToolBlender", "Blender"), COREKIT_BASICUI_TEXT("ToolBlenderBy", "BLENDER FOUNDATION") },
		{ COREKIT_BASICUI_TEXT("ToolPython", "Python"), COREKIT_BASICUI_TEXT("ToolPythonBy", "PYTHON SOFTWARE FOUNDATION") } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Tools); ++Index)
	{
		Line(ToolsColumn, Tools[Index].Left, Tools[Index].Right, Index == UE_ARRAY_COUNT(Tools) - 1);
	}
	UHorizontalBox* Thanks = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddH(Thanks, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta)), 4.f, 0.f), FMargin(0.f), VAlign_Fill);
	UVerticalBox* ThanksText = CoreKitBasicUIBuild::Column(WidgetTree);
	CoreKitBasicUIBuild::AddV(ThanksText, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Thanks", "THANKS FOR PLAYING"), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(28.f), 700, CoreKitBasicUIColor::Magenta)));
	CoreKitBasicUIBuild::AddV(ThanksText, CoreKitBasicUIBuild::Text(WidgetTree, CoreKitBasicUIFrontEnd::GetBuildVersionText(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.14f)), FMargin(0.f, 4.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddH(Thanks, ThanksText, FMargin(16.f, 6.f, 0.f, 6.f), VAlign_Center, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddV(ToolsColumn, Thanks, FMargin(0.f, 34.f, 0.f, 0.f));

	UHorizontalBox* Grid = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Grid"));
	UWidget* Built[] = { TeamColumn, ThirdColumn, ToolsColumn };
	const float Weights[] = { 1.f, 1.15f, 1.f };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Built); ++Index)
	{
		CoreKitBasicUIBuild::AddH(Grid, Built[Index], FMargin(Index == 0 ? 0.f : 28.f, 0.f, Index == 2 ? 0.f : 28.f, 0.f), VAlign_Top, HAlign_Fill, Weights[Index]);
		Columns.Add(Built[Index]);
		CoreKitBasicUIFrontEndUI::Enter(Built[Index], 0.f, FVector2D(0.f, 16.f));
	}
	UCanvasPanelSlot* GridSlot = CoreKitBasicUIBuild::Place(Root, Grid, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector);
	GridSlot->SetOffsets(FMargin(120.f, 222.f, 120.f, 40.f));
}

void UCoreKitBasicUICreditsMenu::NativeOnMenuActivated()
{
	Super::NativeOnMenuActivated();
	if (IntroTime == -1.f)
	{
		IntroTime = 0.f;
	}
}

void UCoreKitBasicUICreditsMenu::TickMotion(float DeltaSeconds)
{
	if (IntroTime < 0.f)
	{
		return;
	}
	IntroTime += DeltaSeconds;
	const UCoreKitBasicUIMotionSettings* Motion = GetDefault<UCoreKitBasicUIMotionSettings>();
	for (int32 Index = 0; Index < Columns.Num(); ++Index)
	{
		CoreKitBasicUIFrontEndUI::Enter(Columns[Index], CoreKitBasicUIFrontEndUI::Step(IntroTime, 0.08f + Motion->DealStagger * Index, Motion->Deal, ECoreKitBasicUIMotion::Deal), FVector2D(0.f, 16.f));
	}
	if (IntroTime > 0.08f + Motion->DealStagger * Columns.Num() + Motion->Deal)
	{
		IntroTime = -2.f;
	}
}

UWidget* UCoreKitBasicUICreditsMenu::GetInitialFocus_Implementation() const
{
	return const_cast<UCoreKitBasicUICreditsMenu*>(this);
}

bool UCoreKitBasicUICreditsMenu::HandleReturn_Implementation()
{
	CoreKitBasicUIFrontEnd::CloseScreen(this);
	return true;
}

void UCoreKitBasicUICreditsMenu::HandleBack(UCoreKitButton*)
{
	CoreKitBasicUIFrontEnd::CloseScreen(this);
}

#undef LOCTEXT_NAMESPACE
