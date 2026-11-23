#include "Widgets/FrontEnd/CoreKitBasicUIFrontEndCommon.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"

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
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "Engine/GameInstance.h"
#include "CoreKitBasicUI/FrontEnd/CoreKitBasicUIMenuGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"

#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIMainMenu"

namespace
{
	enum class EEntry : int32 { Play, HowToPlay, Settings, Credits, Quit };

	UCoreKitFrontendSubsystem* Frontend(const UUserWidget* Widget)
	{
		const UGameInstance* Game = Widget ? Widget->GetGameInstance() : nullptr;
		return Game ? Game->GetSubsystem<UCoreKitFrontendSubsystem>() : nullptr;
	}

	/** Real card presentation when the card asset exists, else the catalog text (Docs/UI/Spec/06-Cards.md). */
	FCoreKitBasicUICardData FanCard(const TCHAR* Id, ECoreKitBasicUICardSuit Suit, bool bSpecial, const FText& Name, const FText& Pre, const FText& Key, const FText& Post)
	{
		FCoreKitBasicUICardData Data;
		Data.Id = Id;
		Data.Name = Name;
		Data.Suit = Suit;
		Data.bSpecial = bSpecial;
		Data.SuitLabel = FText::Format(COREKIT_BASICUI_TEXT("SuitLabel", "{0} · {1}"),
			FText::FromString(Suit == ECoreKitBasicUICardSuit::Sight ? TEXT("SIGHT") : Suit == ECoreKitBasicUICardSuit::Edge ? TEXT("EDGE") : Suit == ECoreKitBasicUICardSuit::Vector ? TEXT("VECTOR") : TEXT("PLATE")),
			FText::FromString(Suit == ECoreKitBasicUICardSuit::Sight ? TEXT("RANGED") : Suit == ECoreKitBasicUICardSuit::Edge ? TEXT("MELEE") : Suit == ECoreKitBasicUICardSuit::Vector ? TEXT("MOBILITY") : TEXT("DEFENSE")));
		Data.EffectPre = Pre;
		Data.EffectKey = Key;
		Data.EffectPost = Post;
		Data.Footer = bSpecial ? COREKIT_BASICUI_TEXT("UniqueFooter", "UNIQUE · LEAVES THE POOL") : COREKIT_BASICUI_TEXT("StackFooter", "STACKS · EVERY COPY ADDS");
		Data.ArtScene = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(FString::Printf(TEXT("/CoreKit/BasicUI/CardArt/T_CardArt_%s_Scene.T_CardArt_%s_Scene"), Id, Id)));
		Data.ArtEffect = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(FString::Printf(TEXT("/CoreKit/BasicUI/CardArt/T_CardArt_%s_Effect.T_CardArt_%s_Effect"), Id, Id)));
		return Data;
	}

	/** Bolt: 10 px Soot circle with a 2 px Shade ring, or a MagentaDark dot on magenta plates. */
	UWidget* MakeBolt(UWidgetTree* Tree, bool bOnMagenta)
	{
		UOverlay* Bolt = CoreKitBasicUIBuild::Overlay(Tree);
		if (bOnMagenta)
		{
			CoreKitBasicUIBuild::AddO(Bolt, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Circle"), CoreKitBasicUIColor::MagentaDark, FVector2D(10.f, 10.f))));
		}
		else
		{
			CoreKitBasicUIBuild::AddO(Bolt, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Circle"), CoreKitBasicUIColor::Soot, FVector2D(10.f, 10.f))));
			CoreKitBasicUIBuild::AddO(Bolt, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Ring2"), CoreKitBasicUIColor::Shade, FVector2D(10.f, 10.f))));
		}
		USizeBox* Box = CoreKitBasicUIBuild::Box(Tree, Bolt, 10.f, 10.f);
		Box->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		return Box;
	}

	/** Hazard tape strip: Soot base with magenta 45-degree stripes. */
	UWidget* MakeTape(UWidgetTree* Tree, int32 Stripe, float Width, float Height)
	{
		UOverlay* Tape = CoreKitBasicUIBuild::Overlay(Tree);
		CoreKitBasicUIBuild::AddO(Tape, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot)));
		CoreKitBasicUIBuild::AddO(Tape, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Tape(Stripe, CoreKitBasicUIColor::Magenta)));
		return CoreKitBasicUIBuild::Box(Tree, Tape, Width, Height);
	}
}

UCoreKitBasicUIMainMenu::UCoreKitBasicUIMainMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowReturn = false;
	bAllowSections = false;
}

TArray<FText> UCoreKitBasicUIMainMenu::GetEntryLabels()
{
	return {
		COREKIT_BASICUI_TEXT("Play", "PLAY"),
		COREKIT_BASICUI_TEXT("HowToPlay", "HOW TO PLAY"),
		COREKIT_BASICUI_TEXT("Settings", "SETTINGS"),
		COREKIT_BASICUI_TEXT("Credits", "CREDITS"),
		COREKIT_BASICUI_TEXT("Quit", "QUIT") };
}

void UCoreKitBasicUIMainMenu::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Background: the arena seen by the menu camera at brightness 0.8, a dark ramp from the left (1300 px) and from
	// the bottom (300 px).
	CoreKitBasicUIBuild::Fill(Root, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.2f)), TEXT("Dim")));
	// Ramps as 25 px solid strips (rgba(10,10,12,.97) -> .88 at 650 px -> 0 at 1300 px; bottom 0 -> .8 over 300 px).
	constexpr float Strip = 25.f;
	for (float X = 0.f; X < 1300.f; X += Strip)
	{
		const float Mid = X + Strip * 0.5f;
		const float Alpha = Mid < 650.f ? FMath::Lerp(0.97f, 0.88f, Mid / 650.f) : FMath::Lerp(0.88f, 0.f, (Mid - 650.f) / 650.f);
		UImage* Band = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(Alpha)));
		UCanvasPanelSlot* BandSlot = CoreKitBasicUIBuild::Place(Root, Band, FAnchors(0.f, 0.f, 0.f, 1.f), FVector2D::ZeroVector, FVector2D(Strip, 1.f));
		BandSlot->SetOffsets(FMargin(X, 0.f, Strip, 0.f));
	}
	for (float Y = 0.f; Y < 300.f; Y += Strip)
	{
		const float Alpha = FMath::Lerp(0.f, 0.8f, (Y + Strip * 0.5f) / 300.f);
		UImage* Band = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(Alpha)));
		UCanvasPanelSlot* BandSlot = CoreKitBasicUIBuild::Place(Root, Band, FAnchors(0.f, 1.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D(1.f, Strip));
		BandSlot->SetOffsets(FMargin(0.f, -300.f + Y, 0.f, Strip));
	}
	FadeIn = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot), TEXT("FadeIn"));
	CoreKitBasicUIBuild::Fill(Root, FadeIn, FMargin(0.f), 1);

	// Decorative fan (anchored right: x 1160, y 170, 620 px on the 1920 artboard).
	Fan = BuildFan();
	CoreKitBasicUIBuild::Place(Root, Fan, FAnchors(1.f, 0.f), FVector2D(-760.f, 170.f), FVector2D(620.f, 620.f), FVector2D::ZeroVector, 2);

	// Title block.
	UVerticalBox* Title = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("TitleBlock"));
	UTextBlock* KickerText = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Kicker", "FPS · ARENA SURVIVAL · ROGUELITE"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(15.f), 500, CoreKitBasicUIColor::Muted, 0.3f), TEXT("Kicker"));
	Kicker = KickerText;
	CoreKitBasicUIBuild::AddV(Title, KickerText, FMargin(0.f, 0.f, 0.f, 22.f), HAlign_Left);
	CoreKitBasicUIBuild::AddV(Title, BuildLogo(), FMargin(0.f, 0.f, 0.f, 22.f), HAlign_Left);
	UHorizontalBox* TaglineRow = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Tagline"));
	const CoreKitBasicUIBuild::FTextStyle TagStyle = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(16.f), 500, CoreKitBasicUIColor::TextDim, 0.18f);
	CoreKitBasicUIBuild::FTextStyle SlashStyle = TagStyle;
	SlashStyle.Color = CoreKitBasicUIColor::Magenta;
	const FText Duration = FText::FromString(CoreKitBasicUIHudLogic::FormatClock(CoreKitBasicUIHudLogic::DefaultSurvivalSeconds()));
	const FText Parts[] = { GetDefault<UCoreKitBasicUISettings>()->Tagline };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Parts); ++Index)
	{
		CoreKitBasicUIBuild::AddH(TaglineRow, CoreKitBasicUIBuild::Text(WidgetTree, Parts[Index], Index % 2 ? SlashStyle : TagStyle));
	}
	Tagline = TaglineRow;
	CoreKitBasicUIBuild::AddV(Title, TaglineRow, FMargin(0.f), HAlign_Left);
	CoreKitBasicUIBuild::Place(Root, Title, FAnchors(0.f, 0.f), FVector2D(120.f, 110.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 3);

	// Menu: PLAY (78 px, focus style, triangle) and four 66 px list entries, gap 12.
	UVerticalBox* Menu = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Menu"));
	const TArray<FText> Labels = GetEntryLabels();
	const FText Sublabels[] = { COREKIT_BASICUI_TEXT("PlaySub", "NEW RUN"), COREKIT_BASICUI_TEXT("HowSub", "CONTROLS · LOOP"), COREKIT_BASICUI_TEXT("SettingsSub", "INPUT · AUDIO · VIDEO"),
		COREKIT_BASICUI_TEXT("CreditsSub", "JAM TEAM"), COREKIT_BASICUI_TEXT("QuitSub", "DESKTOP") };
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		UCoreKitBasicUIActionButton* Entry = WidgetTree->ConstructWidget<UCoreKitBasicUIActionButton>(UCoreKitBasicUIActionButton::StaticClass(), *FString::Printf(TEXT("Entry%d"), Index));
		const bool bPlay = Index == 0;
		Entry->Setup(Labels[Index], Sublabels[Index], bPlay ? ECoreKitBasicUIButtonVariant::Primary : ECoreKitBasicUIButtonVariant::MenuList, bPlay ? 78.f : 66.f, bPlay ? 38.f : 30.f, bPlay);
		Entry->Index = Index;
		Entry->OnActivated.AddUObject(this, &ThisClass::HandleEntry);
		Entry->OnSelected.AddWeakLambda(this, [this](UCoreKitBasicUIActionButton* Selected) { LastEntry = Selected ? Selected->Index : LastEntry; });
		CoreKitBasicUIBuild::AddV(Menu, Entry, FMargin(0.f, Index == 0 ? 0.f : 12.f, 0.f, 0.f));
		Entries.Add(Entry);
	}
	CoreKitBasicUIBuild::Place(Root, CoreKitBasicUIBuild::Box(WidgetTree, Menu, 520.f, 0.f), FAnchors(0.f, 0.f), FVector2D(120.f, 560.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 3);

	// Frontend status (travel errors) under the menu.
	Status = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Red, 0.14f), TEXT("Status"));
	Status->SetVisibility(ESlateVisibility::Collapsed);
	CoreKitBasicUIBuild::Place(Root, Status, FAnchors(0.f, 0.f), FVector2D(120.f, 970.f), FVector2D::ZeroVector, FVector2D::ZeroVector, 3);

	// Mission brief, bottom right (420 px).
	Brief = BuildBrief();
	CoreKitBasicUIBuild::Place(Root, Brief, FAnchors(1.f, 1.f), FVector2D(-120.f, -120.f), FVector2D::ZeroVector, FVector2D(1.f, 1.f), 3);

	// Footer.
	Footer = BuildFooter();
	UCanvasPanelSlot* FooterSlot = CoreKitBasicUIBuild::Place(Root, Footer, FAnchors(0.f, 1.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.f, 1.f), 3);
	FooterSlot->SetOffsets(FMargin(120.f, -44.f, 120.f, 0.f));
	FooterSlot->SetAutoSize(true);

	ApplyIntro(0.f);
}

UWidget* UCoreKitBasicUIMainMenu::BuildLogo()
{
	UVerticalBox* Logo = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Logo"));
	UHorizontalBox* Plates = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Plates"));

	auto MakeWord = [this](const FText& Word, const FLinearColor& Color)
	{
		UTextBlock* Text = CoreKitBasicUIBuild::Text(WidgetTree, Word, CoreKitBasicUIBuild::Stencil(CoreKitBasicUIBuild::Px(150.f), Color));
		Text->SetLineHeightPercentage(0.82f);
		Text->SetApplyLineHeightToBottomLine(true);
		return Text;
	};

	UOverlay* Dead = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("DeadPlate"));
	CoreKitBasicUIBuild::AddO(Dead, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Chamfer(24, CoreKitBasicUIColor::Magenta)));
	CoreKitBasicUIBuild::AddO(Dead, MakeWord(GetDefault<UCoreKitBasicUISettings>()->TitleFirst, CoreKitBasicUIColor::Soot), HAlign_Left, VAlign_Top, FMargin(40.f, 20.f, 34.f, 8.f));
	UWidget* DeadBolt = MakeBolt(WidgetTree, true);
	CoreKitBasicUIBuild::AddO(Dead, DeadBolt, HAlign_Left, VAlign_Bottom, FMargin(12.f, 0.f, 0.f, 10.f));

	UOverlay* Draw = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("DrawPlate"));
	CoreKitBasicUIBuild::AddO(Draw, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::ChamferBottomRight(CoreKitBasicUIColor::Plate)));
	CoreKitBasicUIBuild::AddO(Draw, MakeWord(GetDefault<UCoreKitBasicUISettings>()->TitleSecond, CoreKitBasicUIColor::White), HAlign_Left, VAlign_Top, FMargin(34.f, 20.f, 40.f, 8.f));
	UWidget* DrawBolt = MakeBolt(WidgetTree, false);
	CoreKitBasicUIBuild::AddO(Draw, DrawBolt, HAlign_Right, VAlign_Top, FMargin(0.f, 10.f, 12.f, 0.f));

	CoreKitBasicUIBuild::AddH(Plates, Dead, FMargin(0.f), VAlign_Fill);
	CoreKitBasicUIBuild::AddH(Plates, Draw, FMargin(0.f), VAlign_Fill);
	DeadPlate = Dead;
	DrawPlate = Draw;
	Bolts = { DeadBolt, DrawBolt };
	CoreKitBasicUIBuild::AddV(Logo, Plates, FMargin(0.f), HAlign_Left);

	// Tape + "ARENA UNIT CoreKitBasicUI-10" + tape (14 px stripes, magenta on Soot), 20 px high.
	UHorizontalBox* Tape = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("TapeRow"));
	CoreKitBasicUIBuild::AddH(Tape, MakeTape(WidgetTree, 14, 120.f, 20.f));
	CoreKitBasicUIBuild::AddH(Tape, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Unit", "ARENA UNIT 10"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Steel, 0.3f)), FMargin(14.f, 0.f));
	USizeBox* Filler = Cast<USizeBox>(MakeTape(WidgetTree, 14, 0.f, 20.f));
	Filler->SetMinDesiredWidth(80.f);
	CoreKitBasicUIBuild::AddH(Tape, Filler, FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
	Tape->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	TapeRow = Tape;
	CoreKitBasicUIBuild::AddV(Logo, Tape, FMargin(0.f, 10.f, 0.f, 0.f), HAlign_Fill);
	return Logo;
}

UWidget* UCoreKitBasicUIMainMenu::BuildFan()
{
	UCanvasPanel* FanCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Fan"));
	// Mockup cards are 300 x 430 inside a 620 px box; the Full card is 360 x 540, scaled down to match.
	constexpr float Scale = 0.82f;
	struct FFanCard { FCoreKitBasicUICardData Data; FVector2D Center; float Angle; };
	const FFanCard Cards[] = {
		{ FanCard(TEXT("Firepower"), ECoreKitBasicUICardSuit::Sight, false, COREKIT_BASICUI_TEXT("Firepower", "FIREPOWER"), COREKIT_BASICUI_TEXT("FirepowerPre", ""), COREKIT_BASICUI_TEXT("FirepowerKey", "+15%"), COREKIT_BASICUI_TEXT("FirepowerPost", " rifle damage.")), FVector2D(40.f + 150.f, 90.f + 215.f), -16.f },
		{ FanCard(TEXT("ReinforcedArmor"), ECoreKitBasicUICardSuit::Plate, false, COREKIT_BASICUI_TEXT("Armor", "REINFORCED ARMOR"), COREKIT_BASICUI_TEXT("ArmorPre", ""), COREKIT_BASICUI_TEXT("ArmorKey", "+25"), COREKIT_BASICUI_TEXT("ArmorPost", " max health.")), FVector2D(280.f + 150.f, 90.f + 215.f), 16.f },
		{ FanCard(TEXT("ElectricBlade"), ECoreKitBasicUICardSuit::Edge, true, COREKIT_BASICUI_TEXT("Electric", "ELECTRIC BLADE"), COREKIT_BASICUI_TEXT("ElectricPre", "Katana hits deal bonus "), COREKIT_BASICUI_TEXT("ElectricKey", "electric"), COREKIT_BASICUI_TEXT("ElectricPost", " damage.")), FVector2D(160.f + 150.f, 40.f + 215.f), 0.f },
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cards); ++Index)
	{
		UCoreKitBasicUICardWidget* Card = WidgetTree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass(), *FString::Printf(TEXT("FanCard%d"), Index));
		Card->SetVariant(ECoreKitBasicUICardVariant::Full);
		Card->SetCard(Cards[Index].Data);
		Card->SetState(ECoreKitBasicUICardState::Idle);
		Card->SetVisibility(ESlateVisibility::HitTestInvisible);
		USizeBox* Box = CoreKitBasicUIBuild::Box(WidgetTree, Card, 360.f, 540.f);
		CoreKitBasicUIBuild::Place(FanCanvas, Box, FAnchors(0.f, 0.f), Cards[Index].Center, FVector2D(360.f, 540.f), FVector2D(0.5f, 0.5f), Index);
		Box->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		CoreKitBasicUIBuild::SetTransform(Box, FVector2D::ZeroVector, Scale, Cards[Index].Angle);
		if (Index < 2)
		{
			Box->SetRenderOpacity(0.9f);
		}
	}
	// Magenta glow bar across the fan at -12 degrees.
	UOverlay* Bar = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("GlowBar"));
	CoreKitBasicUIBuild::AddO(Bar, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.7f))), HAlign_Fill, VAlign_Fill, FMargin(0.f, -26.f));
	CoreKitBasicUIBuild::AddO(Bar, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta)));
	CoreKitBasicUIBuild::AddO(Bar, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::WhiteA(0.55f))), 0.f, 2.f), HAlign_Fill, VAlign_Center);
	CoreKitBasicUIBuild::Place(FanCanvas, Bar, FAnchors(0.f, 0.f), FVector2D(-40.f + 360.f, 304.f), FVector2D(720.f, 8.f), FVector2D(0.5f, 0.5f), 5);
	Bar->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Bar->SetRenderTransformAngle(-12.f);
	FanCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	return FanCanvas;
}

UWidget* UCoreKitBasicUIMainMenu::BuildBrief()
{
	UBorder* Panel = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(16, CoreKitBasicUIColor::Hex(0x141418, 0.92f)), FMargin(26.f, 22.f), TEXT("Brief"));
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree);
	Panel->SetContent(Column);
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("BriefTitle", "MISSION BRIEF"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Magenta, 0.22f)));
	URichTextBlock* Body = CoreKitBasicUIFrontEndUI::MakeRich(WidgetTree,
		FText::Format(COREKIT_BASICUI_TEXT("BriefBody", "Hold the arena for <m>{0}</>. Collect what the machines drop. Every level, draw one of three cards. When the clock hits zero, <o>Heavy Mech</> arrives."), FText::FromString(CoreKitBasicUIHudLogic::FormatClock(CoreKitBasicUIHudLogic::DefaultSurvivalSeconds()))),
		CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(21.f), 400, CoreKitBasicUIColor::White));
	CoreKitBasicUIBuild::AddV(Column, Body, FMargin(0.f, 10.f, 0.f, 0.f));
	return CoreKitBasicUIBuild::Box(WidgetTree, Panel, 420.f, 0.f);
}

UWidget* UCoreKitBasicUIMainMenu::BuildFooter()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Footer"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIFrontEndUI::MakeRule(WidgetTree, CoreKitBasicUIColor::Plate));
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddV(Column, Row, FMargin(0.f, 18.f, 0.f, 0.f));

	const CoreKitBasicUIBuild::FTextStyle Label = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.14f);
	auto Key = [this](FKey Keyboard, FKey Pad)
	{
		USizeBox* Box = CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIFrontEndUI::MakeKeycap(WidgetTree, Keyboard, Pad, CoreKitBasicUIColor::Steel), 0.f, 30.f);
		Box->SetMinDesiredWidth(30.f);
		return Box;
	};
	CoreKitBasicUIBuild::AddH(Row, Key(EKeys::W, EKeys::Gamepad_DPad_Up), FMargin(0.f, 0.f, 8.f, 0.f));
	CoreKitBasicUIBuild::AddH(Row, Key(EKeys::S, EKeys::Gamepad_DPad_Down), FMargin(0.f, 0.f, 8.f, 0.f));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Navigate", "NAVIGATE"), Label), FMargin(0.f, 0.f, 28.f, 0.f));
	CoreKitBasicUIBuild::AddH(Row, Key(EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom), FMargin(0.f, 0.f, 8.f, 0.f));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Select", "SELECT"), Label));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), Label), FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, CoreKitBasicUIFrontEnd::GetBuildVersionText(), Label, TEXT("BuildVersion")), FMargin(0.f), VAlign_Center, HAlign_Right);
	return Column;
}

void UCoreKitBasicUIMainMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (UCoreKitFrontendSubsystem* Front = Frontend(this))
	{
		Front->OnChanged.AddUniqueDynamic(this, &ThisClass::HandleFrontendChanged);
	}
	HandleFrontendChanged();
}

void UCoreKitBasicUIMainMenu::NativeDestruct()
{
	if (UCoreKitFrontendSubsystem* Front = Frontend(this))
	{
		Front->OnChanged.RemoveDynamic(this, &ThisClass::HandleFrontendChanged);
	}
	Super::NativeDestruct();
}

void UCoreKitBasicUIMainMenu::HandleFrontendChanged()
{
	const UCoreKitFrontendSubsystem* Front = Frontend(this);
	const FText Message = Front ? Front->GetStatus() : FText::GetEmpty();
	if (Status)
	{
		Status->SetText(Message.ToUpper());
		Status->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Front && Front->GetState() == ECoreKitFrontendState::MainMenu)
	{
		bStarting = false;
	}
}

void UCoreKitBasicUIMainMenu::NativeOnMenuActivated()
{
	Super::NativeOnMenuActivated();
	if (!bIntroPlayed)
	{
		bIntroPlayed = true;
		PlayIntro();
	}
	else
	{
		// Back from a screen: CoreKitNavigation restores PLAY after this call; select the entry we left from.
		bRestoreFocus = true;
	}
}

void UCoreKitBasicUIMainMenu::PlayIntro()
{
	IntroTime = 0.f;
	ApplyIntro(0.f);
}

bool UCoreKitBasicUIMainMenu::IsIntroPlaying() const
{
	return IntroTime >= 0.f;
}

void UCoreKitBasicUIMainMenu::ApplyIntro(float T)
{
	using namespace CoreKitBasicUIFrontEndUI;
	// Motion · Screens, main menu intro (~1.5 s): background 0-400, DEAD 200, DRAW 280, bolts 600, tape 650,
	// tagline 800, entries 850 + 60 ms stagger, footer 1300.
	const UCoreKitBasicUIMotionSettings* Motion = GetDefault<UCoreKitBasicUIMotionSettings>();
	if (FadeIn)
	{
		FadeIn->SetRenderOpacity(1.f - Step(T, 0.f, 0.4f, ECoreKitBasicUIMotion::Linear));
	}
	Enter(Kicker, Step(T, 0.f, 0.4f, ECoreKitBasicUIMotion::Settle), FVector2D(0.f, -12.f));
	// DEAD from the left and DRAW from the right, 80 px, SLAM (the overshoot passes the rest position).
	Enter(DeadPlate, Step(T, 0.2f, Motion->Slam, ECoreKitBasicUIMotion::Slam), FVector2D(-80.f, 0.f));
	Enter(DrawPlate, Step(T, 0.28f, Motion->Slam, ECoreKitBasicUIMotion::Slam), FVector2D(80.f, 0.f));
	const float BoltScale = CoreKitBasicUIMotion::IsReducedMotion() ? (T >= 0.6f ? 1.f : 0.f) : Step(T, 0.6f, 0.3f, ECoreKitBasicUIMotion::Slam);
	for (UWidget* Bolt : Bolts)
	{
		if (Bolt)
		{
			Bolt->SetRenderScale(FVector2D(BoltScale, BoltScale));
		}
	}
	if (TapeRow)
	{
		const float Wipe = Step(T, 0.65f, 0.3f, ECoreKitBasicUIMotion::Settle);
		TapeRow->SetRenderScale(FVector2D(FMath::Max(Wipe, 0.001f), 1.f));
		TapeRow->SetRenderOpacity(Wipe > 0.f ? 1.f : 0.f);
	}
	Enter(Tagline, Step(T, 0.8f, 0.3f, ECoreKitBasicUIMotion::Settle), FVector2D::ZeroVector);
	Enter(Fan, Step(T, 0.6f, Motion->Deal, ECoreKitBasicUIMotion::Deal), FVector2D(24.f, 0.f));
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		Enter(Entries[Index], Step(T, 0.85f + 0.06f * Index, Motion->Deal, ECoreKitBasicUIMotion::Deal), FVector2D(-24.f, 0.f));
	}
	Enter(Brief, Step(T, 1.3f, 0.3f, ECoreKitBasicUIMotion::Settle), FVector2D(0.f, 12.f));
	Enter(Footer, Step(T, 1.3f, 0.3f, ECoreKitBasicUIMotion::Settle), FVector2D::ZeroVector);
}

void UCoreKitBasicUIMainMenu::TickMotion(float DeltaSeconds)
{
	if (bRestoreFocus)
	{
		bRestoreFocus = false;
		if (Entries.IsValidIndex(LastEntry) && Entries[LastEntry])
		{
			Entries[LastEntry]->Select();
		}
	}
	if (IntroTime < 0.f)
	{
		return;
	}
	IntroTime += DeltaSeconds;
	constexpr float IntroEnd = 1.75f;
	if (IntroTime >= IntroEnd)
	{
		IntroTime = -1.f;
		ApplyIntro(10.f);
		return;
	}
	ApplyIntro(IntroTime);
}

UWidget* UCoreKitBasicUIMainMenu::GetInitialFocus_Implementation() const
{
	return Entries.Num() > 0 && Entries[0] ? Entries[0]->GetButton() : nullptr;
}

bool UCoreKitBasicUIMainMenu::HandleReturn_Implementation()
{
	// The title screen has nothing to return to; Back only skips the intro.
	if (!IsIntroPlaying())
	{
		return false;
	}
	IntroTime = -1.f;
	ApplyIntro(10.f);
	return true;
}

bool UCoreKitBasicUIMainMenu::HandleMenuKey_Implementation(FKey Key)
{
	if (Key == EKeys::W)
	{
		MoveSelection(-1);
		return true;
	}
	if (Key == EKeys::S)
	{
		MoveSelection(1);
		return true;
	}
	return false;
}

void UCoreKitBasicUIMainMenu::MoveSelection(int32 Delta)
{
	int32 Current = 0;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index] && Entries[Index]->IsSelected())
		{
			Current = Index;
		}
	}
	const int32 Next = FMath::Clamp(Current + Delta, 0, Entries.Num() - 1);
	if (Entries.IsValidIndex(Next) && Entries[Next])
	{
		Entries[Next]->Select();
	}
}

void UCoreKitBasicUIMainMenu::HandleEntry(UCoreKitBasicUIActionButton* Button)
{
	if (!Button || bStarting)
	{
		return;
	}
	APlayerController* Controller = GetOwningPlayer();
	switch (static_cast<EEntry>(Button->Index))
	{
	case EEntry::Play:
		StartNewRun();
		break;
	case EEntry::HowToPlay:
		CoreKitBasicUIFrontEnd::OpenHowToPlay(Controller);
		break;
	case EEntry::Settings:
		CoreKitBasicUIFrontEnd::OpenSettings(Controller);
		break;
	case EEntry::Credits:
		CoreKitBasicUIFrontEnd::OpenCredits(Controller);
		break;
	case EEntry::Quit:
	{
		TWeakObjectPtr<UCoreKitBasicUIMainMenu> WeakThis(this);
		CoreKitBasicUIFrontEndUI::Confirm(this, ECoreKitBasicUIPromptPreset::QuitToDesktop,
			COREKIT_BASICUI_TEXT("QuitBody", "The session closes. Settings and key bindings are already saved."),
			[WeakThis](bool bConfirmed)
			{
				if (bConfirmed && WeakThis.IsValid())
				{
					UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis->GetOwningPlayer(), EQuitPreference::Quit, false);
				}
			});
		break;
	}
	}
}

void UCoreKitBasicUIMainMenu::StartNewRun()
{
	UCoreKitFrontendSubsystem* Front = Frontend(this);
	if (Front && Front->GetState() != ECoreKitFrontendState::MainMenu)
	{
		return;
	}
	bStarting = true;
	if (ACoreKitBasicUIMenuPlayerController* Controller = Cast<ACoreKitBasicUIMenuPlayerController>(GetOwningPlayer()))
	{
		Controller->FadeOutMusic(UCoreKitBasicUIScreenWipe::GetDuration());
	}
	// Cover wipe first; the loading screen (UCoreKitBasicUILoadingScreen) continues on the covered frame.
	TWeakObjectPtr<UCoreKitFrontendSubsystem> WeakFront(Front);
	TWeakObjectPtr<UCoreKitBasicUIMainMenu> WeakThis(this);
	const bool bHasFrontend = Front != nullptr;
	UCoreKitBasicUIScreenWipe::Play(GetOwningPlayer(), ECoreKitBasicUIWipeMode::Cover, [WeakFront, WeakThis, bHasFrontend]()
	{
		if (UCoreKitFrontendSubsystem* Target = WeakFront.Get())
		{
			Target->NewGame();
		}
		else if (!bHasFrontend && WeakThis.IsValid())
		{
			// CoreKitFrontend disabled: plain travel to the arena.
			const FSoftObjectPath Map = GetDefault<UCoreKitBasicUISettings>()->GameplayMap.ToSoftObjectPath();
			if (Map.IsValid())
			{
				WeakThis->CloseMenu();
				UGameplayStatics::OpenLevel(WeakThis.Get(), FName(*Map.GetLongPackageName()));
			}
			else { WeakThis->bStarting = false; WeakThis->HandleFrontendChanged(); UCoreKitBasicUIScreenWipe::Play(WeakThis->GetOwningPlayer(), ECoreKitBasicUIWipeMode::Reveal); }
		}
	});
}

#undef LOCTEXT_NAMESPACE
