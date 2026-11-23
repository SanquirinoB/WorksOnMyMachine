#include "Widgets/FrontEnd/CoreKitBasicUIFrontEndCommon.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUIUserSettings.h"

#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIHowToPlay"

namespace
{
	/** Heal amount of the repair kit from the drone data (BP_CoreKitBasicUIHoverMelee drops), else the component default. */
	int32 RepairKitHealth() { return GetDefault<UCoreKitBasicUISettings>()->ExampleHealAmount; }

	/** Mockup grid 1fr 1.15fr 1fr with 56 px gaps inside 1680 px. */
	constexpr float ColumnWidths[] = { 497.8f, 572.4f, 497.8f };
}

UCoreKitBasicUIHowToPlayMenu::UCoreKitBasicUIHowToPlayMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowSections = false;
	SetIsFocusable(true);
}

UInputAction* UCoreKitBasicUIHowToPlayMenu::LoadAction(FName ActionName)
{
	UInputAction* Action = GetDefault<UCoreKitBasicUISettings>()->FindAction(ActionName);
	if (Action)
	{
		Actions.AddUnique(Action);
	}
	return Action;
}

UCoreKitBasicUIKeycap* UCoreKitBasicUIHowToPlayMenu::ActionKeycap(FName ActionName, FKey Fallback, const FLinearColor& Accent)
{
	// Current binding of the action for the active device; fixed key when the action asset does not exist yet.
	UCoreKitBasicUIKeycap* Keycap = WidgetTree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass());
	Keycap->SetAccent(Accent);
	// Fixed key when the action asset does not exist yet or IMC_Combat does not map it yet.
	const UInputMappingContext* Mapping = GetDefault<UCoreKitBasicUISettings>()->InputMapping.LoadSynchronous();
	UInputAction* Action = LoadAction(ActionName);
	const bool bMapped = Action && Mapping && Mapping->GetMappings().ContainsByPredicate([Action](const FEnhancedActionKeyMapping& Item) { return Item.Action == Action; });
	if (bMapped)
	{
		Keycap->SetAction(Action);
	}
	else
	{
		Keycap->SetKey(Fallback);
	}
	return Keycap;
}

UWidget* UCoreKitBasicUIHowToPlayMenu::ControlRow(const FText& Label, UWidget* Keys, bool bLast, const FLinearColor& LabelColor)
{
	UVerticalBox* Row = CoreKitBasicUIBuild::Column(WidgetTree);
	UHorizontalBox* Line = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddH(Line, CoreKitBasicUIBuild::Text(WidgetTree, Label, CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(22.f), 600, LabelColor)), FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
	USizeBox* KeyBox = CoreKitBasicUIBuild::Box(WidgetTree, Keys, 0.f, 40.f);
	CoreKitBasicUIBuild::AddH(Line, KeyBox, FMargin(16.f, 0.f, 0.f, 0.f), VAlign_Center, HAlign_Right);
	CoreKitBasicUIBuild::AddV(Row, Line, FMargin(0.f, 11.f));
	if (!bLast)
	{
		CoreKitBasicUIBuild::AddV(Row, CoreKitBasicUIFrontEndUI::MakeRule(WidgetTree, CoreKitBasicUIColor::Plate));
	}
	return Row;
}

UWidget* UCoreKitBasicUIHowToPlayMenu::BuildControls()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Controls"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("Controls", "Controls")), FMargin(0.f, 0.f, 0.f, 14.f));

	auto Keys = [this](std::initializer_list<UWidget*> Items)
	{
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		bool bFirst = true;
		for (UWidget* Item : Items)
		{
			USizeBox* Box = CoreKitBasicUIBuild::Box(WidgetTree, Item, 0.f, 40.f);
			Box->SetMinDesiredWidth(40.f);
			CoreKitBasicUIBuild::AddH(Row, Box, FMargin(bFirst ? 0.f : 6.f, 0.f, 0.f, 0.f));
			bFirst = false;
		}
		return Row;
	};
	auto Fixed = [this](FKey Keyboard, FKey Pad, const FLinearColor& Accent = CoreKitBasicUIColor::Steel)
	{
		return CoreKitBasicUIFrontEndUI::MakeKeycap(WidgetTree, Keyboard, Pad, Accent);
	};

	auto FixedKeyboardOnly = [this](FKey Keyboard)
	{
		UCoreKitBasicUIDeviceKeycap* Keycap = CoreKitBasicUIFrontEndUI::MakeKeycap(WidgetTree, Keyboard, FKey());
		Keycap->SetHiddenOnGamepad(true);
		return Keycap;
	};
	// Move and Look are fixed (axes): WASD / mouse, left / right stick.
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Move", "Move"), Keys({ Fixed(EKeys::W, EKeys::Gamepad_Left2D), FixedKeyboardOnly(EKeys::A), FixedKeyboardOnly(EKeys::S), FixedKeyboardOnly(EKeys::D) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Look", "Look"), Keys({ Fixed(EKeys::Mouse2D, EKeys::Gamepad_Right2D) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Jump", "Jump"), Keys({ ActionKeycap(TEXT("IA_Jump"), EKeys::SpaceBar) })));

	UHorizontalBox* Dash = CoreKitBasicUIBuild::Row(WidgetTree);
	USizeBox* ShiftBox = CoreKitBasicUIBuild::Box(WidgetTree, ActionKeycap(TEXT("IA_Dash"), EKeys::LeftShift, CoreKitBasicUIColor::Magenta), 0.f, 40.f);
	ShiftBox->SetMinDesiredWidth(40.f);
	CoreKitBasicUIBuild::AddH(Dash, ShiftBox);
	CoreKitBasicUIBuild::AddH(Dash, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Plus", "+"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(15.f), 500, CoreKitBasicUIColor::Muted, 0.f)), FMargin(6.f, 0.f));
	UCoreKitBasicUIKeycap* Dir = WidgetTree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass());
	Dir->SetFixedText(COREKIT_BASICUI_TEXT("Dir", "DIR"));
	USizeBox* DirBox = CoreKitBasicUIBuild::Box(WidgetTree, Dir, 0.f, 40.f);
	DirBox->SetMinDesiredWidth(40.f);
	CoreKitBasicUIBuild::AddH(Dash, DirBox);
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Dash", "Dash"), Dash, false, CoreKitBasicUIColor::Magenta));

	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Attack", "Attack"), Keys({ ActionKeycap(TEXT("IA_Attack"), EKeys::LeftMouseButton) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Reload", "Reload"), Keys({ ActionKeycap(TEXT("IA_Reload"), EKeys::R) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Weapons", "Rifle / Katana"), Keys({ ActionKeycap(TEXT("IA_Primary"), EKeys::One, CoreKitBasicUIColor::White), ActionKeycap(TEXT("IA_Secondary"), EKeys::Two, CoreKitBasicUIColor::Magenta) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Swap", "Swap weapon"), Keys({ ActionKeycap(TEXT("IA_WeaponWheel"), EKeys::MouseScrollUp) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Inspect", "Inspect build"), Keys({ ActionKeycap(TEXT("IA_InspectBuild"), EKeys::Tab) })));
	CoreKitBasicUIBuild::AddV(Column, ControlRow(COREKIT_BASICUI_TEXT("Pause", "Pause"), Keys({ Fixed(EKeys::Escape, EKeys::Gamepad_Special_Right) }), true));

	DashNote = CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(18.f), 400, CoreKitBasicUIColor::Steel));
	CoreKitBasicUIBuild::AddV(Column, DashNote, FMargin(0.f, 10.f, 0.f, 0.f));
	return Column;
}

UWidget* UCoreKitBasicUIHowToPlayMenu::BuildLoop()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Loop"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("TheLoop", "The loop")), FMargin(0.f, 0.f, 0.f, 14.f));
	struct FStep { const TCHAR* Number; FLinearColor Color; FText Title; FText Body; };
	const FStep Steps[] = {
		{ TEXT("01"), CoreKitBasicUIColor::White, COREKIT_BASICUI_TEXT("Fight", "FIGHT"), COREKIT_BASICUI_TEXT("FightBody", "Rifle at range, katana up close. Switch any time.") },
		{ TEXT("02"), CoreKitBasicUIColor::Lime, COREKIT_BASICUI_TEXT("Collect", "COLLECT"), COREKIT_BASICUI_TEXT("CollectBody", "XP orbs and heals stay on the floor. Go in and take them.") },
		{ TEXT("03"), CoreKitBasicUIColor::Magenta, COREKIT_BASICUI_TEXT("Draw", "DRAW"), COREKIT_BASICUI_TEXT("DrawBody", "Fill the XP bar and time stops. Pick one of three cards.") },
		{ TEXT("04"), CoreKitBasicUIColor::Orange, COREKIT_BASICUI_TEXT("Survive", "SURVIVE"), FText::Format(COREKIT_BASICUI_TEXT("SurviveBody", "At {0} Heavy Mech drops in. Destroy it to win the run."), FText::FromString(CoreKitBasicUIHudLogic::FormatClock(CoreKitBasicUIHudLogic::DefaultSurvivalSeconds()))) },
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Steps); ++Index)
	{
		UBorder* Plate = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(14, CoreKitBasicUIColor::Hull), FMargin(22.f, 16.f));
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		Plate->SetContent(Row);
		UTextBlock* Number = CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(Steps[Index].Number), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(56.f), 700, Steps[Index].Color));
		Number->SetLineHeightPercentage(0.9f);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(WidgetTree, Number, 90.f, 0.f), FMargin(0.f, 0.f, 18.f, 0.f));
		UVerticalBox* Text = CoreKitBasicUIBuild::Column(WidgetTree);
		CoreKitBasicUIBuild::AddV(Text, CoreKitBasicUIBuild::Text(WidgetTree, Steps[Index].Title, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(28.f), 700)));
		CoreKitBasicUIBuild::AddV(Text, CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree, Steps[Index].Body, CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(19.f), 400, CoreKitBasicUIColor::TextDim)));
		CoreKitBasicUIBuild::AddH(Row, Text, FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
		CoreKitBasicUIBuild::AddV(Column, Plate, FMargin(0.f, Index == 0 ? 0.f : 14.f, 0.f, 0.f));
	}

	UHorizontalBox* Weapons = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Weapons"));
	auto Weapon = [this](const FText& Name, const FText& Body, const FLinearColor& Color)
	{
		UHorizontalBox* Item = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddH(Item, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(Color)), 4.f, 0.f), FMargin(0.f), VAlign_Fill);
		UVerticalBox* Text = CoreKitBasicUIBuild::Column(WidgetTree);
		CoreKitBasicUIBuild::AddV(Text, CoreKitBasicUIBuild::Text(WidgetTree, Name, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(22.f), 700, Color)));
		CoreKitBasicUIBuild::AddV(Text, CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree, Body, CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(18.f), 400, CoreKitBasicUIColor::TextDim)));
		CoreKitBasicUIBuild::AddH(Item, Text, FMargin(16.f, 6.f, 0.f, 6.f), VAlign_Center, HAlign_Fill, 1.f);
		return Item;
	};
	CoreKitBasicUIBuild::AddH(Weapons, Weapon(COREKIT_BASICUI_TEXT("Rifle", "ASSAULT RIFLE"), COREKIT_BASICUI_TEXT("RifleBody", "Medium range. Full auto. Watch your magazine."), CoreKitBasicUIColor::White), FMargin(0.f, 0.f, 7.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(Weapons, Weapon(COREKIT_BASICUI_TEXT("Katana", "ENERGY KATANA"), COREKIT_BASICUI_TEXT("KatanaBody", "Close range. Heavy damage. Never reloads."), CoreKitBasicUIColor::Magenta), FMargin(7.f, 0.f, 0.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddV(Column, Weapons, FMargin(0.f, 20.f, 0.f, 0.f));
	return Column;
}

UWidget* UCoreKitBasicUIHowToPlayMenu::BuildGlow()
{
	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Glow"));
	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("ReadTheGlow", "Read the glow")), FMargin(0.f, 0.f, 0.f, 14.f));

	auto Item = [this, Column](UWidget* Icon, const FText& Title, const FText& Body, const FLinearColor& Color, bool bFirst)
	{
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(WidgetTree, Icon, 52.f, 52.f), FMargin(0.f, 0.f, 18.f, 0.f));
		UVerticalBox* Text = CoreKitBasicUIBuild::Column(WidgetTree);
		CoreKitBasicUIBuild::AddV(Text, CoreKitBasicUIBuild::Text(WidgetTree, Title, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(22.f), 700, Color)));
		CoreKitBasicUIBuild::AddV(Text, CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree, Body, CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(18.f), 400, CoreKitBasicUIColor::TextDim)));
		CoreKitBasicUIBuild::AddH(Row, Text, FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
		CoreKitBasicUIBuild::AddV(Column, Row, FMargin(0.f, bFirst ? 0.f : 16.f, 0.f, 0.f));
	};
	auto GlowIcon = [this](FName Texture, const FLinearColor& Color, float Size, float Angle = 0.f, FName Ring = NAME_None)
	{
		UOverlay* Icon = CoreKitBasicUIBuild::Overlay(WidgetTree);
		CoreKitBasicUIBuild::AddO(Icon, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(Color, 0.55f), FVector2D(52.f, 52.f))), HAlign_Center, VAlign_Center);
		if (!Ring.IsNone())
		{
			CoreKitBasicUIBuild::AddO(Icon, CoreKitBasicUIBuild::Icon(WidgetTree, Ring, Color, 40.f), HAlign_Center, VAlign_Center);
		}
		UImage* Glyph = CoreKitBasicUIBuild::Icon(WidgetTree, Texture, Color, Size);
		Glyph->SetRenderTransformAngle(Angle);
		CoreKitBasicUIBuild::AddO(Icon, Glyph, HAlign_Center, VAlign_Center);
		return Icon;
	};
	Item(GlowIcon(TEXT("T_Icon_Diamond"), CoreKitBasicUIColor::Lime, 30.f), COREKIT_BASICUI_TEXT("XP", "LIME · XP ORB"), COREKIT_BASICUI_TEXT("XPBody", "Dropped by every kill. Fills your next draw."), CoreKitBasicUIColor::Lime, true);
	Item(GlowIcon(TEXT("T_Icon_Cross"), CoreKitBasicUIColor::Mint, 40.f), COREKIT_BASICUI_TEXT("Repair", "MINT · REPAIR KIT"),
		FText::Format(COREKIT_BASICUI_TEXT("RepairBody", "Sometimes dropped. Restores {0} HP."), FText::AsNumber(RepairKitHealth())), CoreKitBasicUIColor::Mint, false);
	Item(GlowIcon(TEXT("T_UI_Circle"), CoreKitBasicUIColor::Orange, 14.f, 0.f, TEXT("T_UI_Ring4")), COREKIT_BASICUI_TEXT("Incoming", "ORANGE · INCOMING"), COREKIT_BASICUI_TEXT("IncomingBody", "Charging shots, slam zones. Move or dash."), CoreKitBasicUIColor::Orange, false);
	Item(GlowIcon(TEXT("T_Icon_Chevrons"), CoreKitBasicUIColor::Magenta, 40.f), COREKIT_BASICUI_TEXT("You", "MAGENTA · YOU"), COREKIT_BASICUI_TEXT("YouBody", "Your blade, your dash, your next card."), CoreKitBasicUIColor::Magenta, false);

	CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIFrontEndUI::MakeHeading(WidgetTree, COREKIT_BASICUI_TEXT("Suits", "Card suits")), FMargin(0.f, 34.f, 0.f, 14.f));
	UHorizontalBox* Suits = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Suits"));
	struct FSuit { ECoreKitBasicUICardSuit Suit; FText Domain; };
	const FSuit Items[] = {
		{ ECoreKitBasicUICardSuit::Sight, COREKIT_BASICUI_TEXT("SuitRifle", "RIFLE") }, { ECoreKitBasicUICardSuit::Edge, COREKIT_BASICUI_TEXT("SuitBlade", "BLADE") },
		{ ECoreKitBasicUICardSuit::Vector, COREKIT_BASICUI_TEXT("SuitDash", "DASH") }, { ECoreKitBasicUICardSuit::Plate, COREKIT_BASICUI_TEXT("SuitArmor", "ARMOR") } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Items); ++Index)
	{
		const FLinearColor Color = UCoreKitBasicUIStyle::SuitColor(Items[Index].Suit);
		UOverlay* Tile = CoreKitBasicUIBuild::Overlay(WidgetTree);
		CoreKitBasicUIBuild::AddO(Tile, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Hull)));
		CoreKitBasicUIBuild::AddO(Tile, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(Color)), 0.f, 3.f), HAlign_Fill, VAlign_Top);
		UVerticalBox* Content = CoreKitBasicUIBuild::Column(WidgetTree);
		CoreKitBasicUIBuild::AddV(Content, CoreKitBasicUIBuild::Icon(WidgetTree, UCoreKitBasicUIStyle::SuitIcon(Items[Index].Suit), Color, 36.f), FMargin(0.f), HAlign_Center);
		CoreKitBasicUIBuild::AddV(Content, CoreKitBasicUIBuild::Text(WidgetTree, Items[Index].Domain, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::White, 0.14f)), FMargin(0.f, 8.f, 0.f, 0.f), HAlign_Center);
		CoreKitBasicUIBuild::AddO(Tile, Content, HAlign_Center, VAlign_Center, FMargin(8.f, 17.f, 8.f, 14.f));
		CoreKitBasicUIBuild::AddH(Suits, Tile, FMargin(Index == 0 ? 0.f : 5.f, 0.f, Index == 3 ? 0.f : 5.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);
	}
	CoreKitBasicUIBuild::AddV(Column, Suits);
	return Column;
}

void UCoreKitBasicUIHowToPlayMenu::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIFrontEndUI::MakeScreenRoot(WidgetTree);

	// Header: FIELD MANUAL / HOW TO PLAY, ESC BACK on the right.
	UHorizontalBox* Header = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Header"));
	CoreKitBasicUIBuild::AddH(Header, CoreKitBasicUIFrontEndUI::MakeTitleBlock(WidgetTree, COREKIT_BASICUI_TEXT("Kicker", "FIELD MANUAL"), COREKIT_BASICUI_TEXT("Title", "How to play")), FMargin(0.f), VAlign_Bottom, HAlign_Left, 1.f);
	UCoreKitButton* Back = nullptr;
	CoreKitBasicUIBuild::AddH(Header, CoreKitBasicUIFrontEndUI::MakeKeyHint(WidgetTree, EKeys::Escape, EKeys::Gamepad_FaceButton_Right, COREKIT_BASICUI_TEXT("Back", "BACK"), CoreKitBasicUIColor::TextDim, CoreKitBasicUIColor::Steel, &Back), FMargin(0.f, 0.f, 0.f, 8.f), VAlign_Bottom);
	BackButton = Back;
	Back->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleBack);
	UCanvasPanelSlot* HeaderSlot = CoreKitBasicUIBuild::Place(Root, Header, FAnchors(0.f, 0.f, 1.f, 0.f), FVector2D::ZeroVector);
	HeaderSlot->SetOffsets(FMargin(120.f, 64.f, 120.f, 0.f));
	HeaderSlot->SetAutoSize(true);

	// Three columns (1fr 1.15fr 1fr, gap 56).
	UHorizontalBox* Grid = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Grid"));
	UWidget* Built[] = { BuildControls(), BuildLoop(), BuildGlow() };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Built); ++Index)
	{
		CoreKitBasicUIBuild::AddH(Grid, Built[Index], FMargin(Index == 0 ? 0.f : 28.f, 0.f, Index == 2 ? 0.f : 28.f, 0.f), VAlign_Top, HAlign_Fill, ColumnWidths[Index]);
		Columns.Add(Built[Index]);
		CoreKitBasicUIFrontEndUI::Enter(Built[Index], 0.f, FVector2D(0.f, 16.f));
	}
	UCanvasPanelSlot* GridSlot = CoreKitBasicUIBuild::Place(Root, Grid, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector);
	GridSlot->SetOffsets(FMargin(120.f, 222.f, 120.f, 40.f));
}

void UCoreKitBasicUIHowToPlayMenu::NativeConstruct()
{
	Super::NativeConstruct();
	// "Dash with no direction pressed goes forward" follows the Settings value (BACKWARD changes the text).
	if (DashNote)
	{
		const bool bBackward = UCoreKitBasicUIUserSettings::Get()->bDashBackwardWithoutInput;
		DashNote->SetText(bBackward
			? COREKIT_BASICUI_TEXT("DashNoteBack", "Dash with no direction pressed goes backward. You can attack while dashing.")
			: COREKIT_BASICUI_TEXT("DashNoteForward", "Dash with no direction pressed goes forward. You can attack while dashing."));
	}
}

void UCoreKitBasicUIHowToPlayMenu::NativeOnMenuActivated()
{
	Super::NativeOnMenuActivated();
	// First activation plays the columns in (-1 = not started, -2 = done).
	if (IntroTime == -1.f)
	{
		IntroTime = 0.f;
	}
}

void UCoreKitBasicUIHowToPlayMenu::TickMotion(float DeltaSeconds)
{
	// Columns deal in from below (16 px, DEAL + 70 ms stagger).
	if (IntroTime < 0.f)
	{
		return;
	}
	IntroTime += DeltaSeconds;
	const float Deal = GetDefault<UCoreKitBasicUIMotionSettings>()->Deal;
	const float Stagger = GetDefault<UCoreKitBasicUIMotionSettings>()->DealStagger;
	for (int32 Index = 0; Index < Columns.Num(); ++Index)
	{
		CoreKitBasicUIFrontEndUI::Enter(Columns[Index], CoreKitBasicUIFrontEndUI::Step(IntroTime, 0.08f + Stagger * Index, Deal, ECoreKitBasicUIMotion::Deal), FVector2D(0.f, 16.f));
	}
	if (IntroTime > 0.08f + Stagger * Columns.Num() + Deal)
	{
		IntroTime = -2.f;
	}
}

UWidget* UCoreKitBasicUIHowToPlayMenu::GetInitialFocus_Implementation() const
{
	// Nothing to select: the screen itself takes focus (so the title screen below keeps none).
	return const_cast<UCoreKitBasicUIHowToPlayMenu*>(this);
}

bool UCoreKitBasicUIHowToPlayMenu::HandleReturn_Implementation()
{
	CoreKitBasicUIFrontEnd::CloseScreen(this);
	return true;
}

void UCoreKitBasicUIHowToPlayMenu::HandleBack(UCoreKitButton*)
{
	CoreKitBasicUIFrontEnd::CloseScreen(this);
}

#undef LOCTEXT_NAMESPACE
