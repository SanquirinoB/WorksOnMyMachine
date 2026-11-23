#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUICardDrawMenu.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitBasicUI/Effects/CoreKitBasicUIScreenEffectsComponent.h"
#include "Engine/LocalPlayer.h"
#include "CoreKitBasicUI/Flow/CoreKitBasicUIScreenRouter.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUIViewModels.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUICardDraw"

namespace
{
	enum EStep : uint8
	{
		StepXP = 1 << 0,
		StepFlash = 1 << 1,
		StepTitle = 1 << 2,
		StepDeal = 1 << 3,
		StepChoose = 1 << 4
	};

	constexpr float CardWidth = 360.f;
	constexpr float CardHeight = 540.f;
	constexpr float CardGap = 56.f;
	constexpr float CardTop = 290.f;
	constexpr float DealRotation = 18.f;

	/** Slash line with a soft magenta glow, stretched across the screen; pivot on its left end. */
	UCanvasPanel* MakeSlash(UWidgetTree* Tree, UCanvasPanel* Parent, float Y, float Thickness, float Angle, FName Name)
	{
		UCanvasPanel* Slash = CoreKitBasicUIBuild::Canvas(Tree, Name);
		Slash->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* SlashSlot = Parent->AddChildToCanvas(Slash);
		SlashSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		SlashSlot->SetOffsets(FMargin(-100.f, Y - 20.f, -100.f, 40.f));
		SlashSlot->SetZOrder(4);
		UImage* Halo = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.55f)));
		CoreKitBasicUIBuild::Fill(Slash, Halo);
		UImage* Line = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta));
		UCanvasPanelSlot* LineSlot = Slash->AddChildToCanvas(Line);
		LineSlot->SetAnchors(FAnchors(0.f, 0.5f, 1.f, 0.5f));
		LineSlot->SetOffsets(FMargin(0.f, -Thickness * 0.5f, 0.f, Thickness));
		Slash->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		Slash->SetRenderTransformAngle(Angle);
		return Slash;
	}

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, const CoreKitBasicUIBuild::FTextStyle& Style, FName Name = NAME_None)
	{
		UTextBlock* Block = CoreKitBasicUIBuild::Text(Tree, Text, Style, Name);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	}

	FKey CardKey(int32 Index)
	{
		switch (Index)
		{
		case 0: return EKeys::One;
		case 1: return EKeys::Two;
		case 2: return EKeys::Three;
		default: return FKey();
		}
	}
}

UCoreKitBasicUICardDrawMenu::UCoreKitBasicUICardDrawMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowReturn = false;
	bAllowSections = false;
	// D6: 400 ms lock. CoreKit's cooldown covers the push; FCoreKitBasicUICardDrawFlow restarts the lock when the cards land.
	ActivationInputCooldown = 0.4f;
}

void UCoreKitBasicUICardDrawMenu::SetViewModels(UCoreKitBasicUIProgressionViewModel* InProgression, UCoreKitBasicUIBuildViewModel* InBuild)
{
	ProgressionOverride = InProgression;
	BuildOverride = InBuild;
}

UCoreKitBasicUIProgressionViewModel* UCoreKitBasicUICardDrawMenu::Progression() const
{
	if (ProgressionOverride)
	{
		return ProgressionOverride;
	}
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	return Context ? Context->GetProgression() : nullptr;
}

UCoreKitBasicUIBuildViewModel* UCoreKitBasicUICardDrawMenu::Build() const
{
	if (BuildOverride)
	{
		return BuildOverride;
	}
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	return Context ? Context->GetBuild() : nullptr;
}

void UCoreKitBasicUICardDrawMenu::BuildTree()
{
	UWidgetTree* Tree = WidgetTree;
	RootCanvas = CoreKitBasicUIBuild::Canvas(Tree, TEXT("CardDrawRoot"));
	Tree->RootWidget = RootCanvas;

	// World freeze: blur 10 px (animated), dark veil + scanlines; the post-process freeze desaturates.
	Blur = Tree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("WorldBlur"));
	Blur->SetBlurStrength(0.f);
	Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoreKitBasicUIBuild::Fill(RootCanvas, Blur, FMargin(0.f), 0);
	UCanvasPanel* Veil = CoreKitBasicUIBuild::Canvas(Tree, TEXT("VeilGroup"));
	Veil->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoreKitBasicUIBuild::Fill(RootCanvas, Veil, FMargin(0.f), 1);
	CoreKitBasicUIBuild::Fill(Veil, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.5f))));
	CoreKitBasicUIBuild::Fill(Veil, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Scanlines(0.03f)));
	Veil->SetRenderOpacity(0.f);
	VeilGroup = Veil;

	Flash = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(FLinearColor::White), TEXT("Flash"));
	Flash->SetVisibility(ESlateVisibility::HitTestInvisible);
	Flash->SetRenderOpacity(0.f);
	CoreKitBasicUIBuild::Fill(RootCanvas, Flash, FMargin(0.f), 30);

	// Slashes: -4 deg at y 470 during the title intro, -3 deg at y 186 (opacity .5) while choosing.
	IntroSlash = MakeSlash(Tree, RootCanvas, 470.f, 5.f, -4.f, TEXT("IntroSlash"));
	ChooseSlash = MakeSlash(Tree, RootCanvas, 186.f, 4.f, -3.f, TEXT("ChooseSlash"));
	IntroSlash->SetRenderOpacity(0.f);
	ChooseSlash->SetRenderOpacity(0.f);

	// Intro title "LEVEL UP · DRAW" (150 px, 800) with "LEVEL nn" above.
	UVerticalBox* Intro = CoreKitBasicUIBuild::Column(Tree, TEXT("IntroTitle"));
	Intro->SetVisibility(ESlateVisibility::HitTestInvisible);
	IntroLevel = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::WithShadow(CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), 500, CoreKitBasicUIColor::Lime, 0.32f)), TEXT("IntroLevel"));
	CoreKitBasicUIBuild::AddV(Intro, IntroLevel, FMargin(0.f, 0.f, 0.f, 4.f), HAlign_Center);
	UHorizontalBox* IntroRow = CoreKitBasicUIBuild::Row(Tree);
	CoreKitBasicUIBuild::AddH(IntroRow, Label(Tree, COREKIT_BASICUI_TEXT("LevelUp", "LEVEL UP "), CoreKitBasicUIBuild::WithShadow(CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(150.f), 800))));
	CoreKitBasicUIBuild::AddH(IntroRow, Label(Tree, COREKIT_BASICUI_TEXT("Draw", "· DRAW"), CoreKitBasicUIBuild::WithShadow(CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(150.f), 800, CoreKitBasicUIColor::Magenta))));
	CoreKitBasicUIBuild::AddV(Intro, IntroRow, FMargin(0.f), HAlign_Center);
	CoreKitBasicUIBuild::Place(RootCanvas, Intro, FAnchors(0.5f, 0.f), FVector2D(0.f, 360.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f), 20);
	Intro->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
	IntroTitle = Intro;

	// Header of the choice state: chips + "DRAW ONE" (104 px, "ONE" magenta).
	UVerticalBox* HeaderBox = CoreKitBasicUIBuild::Column(Tree, TEXT("Header"));
	HeaderBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	UHorizontalBox* Chips = CoreKitBasicUIBuild::Row(Tree, TEXT("Chips"));
	LevelChip = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Lime, 0.3f), TEXT("LevelChip"));
	CoreKitBasicUIBuild::AddH(Chips, LevelChip, FMargin(0.f, 0.f, 12.f, 0.f));
	UBorder* Frozen = CoreKitBasicUIBuild::Panel(Tree, CoreKitBasicUIOverlay::Outline(CoreKitBasicUIColor::Steel, 1.f), FMargin(10.f, 4.f), TEXT("FrozenChip"));
	Frozen->SetContent(Label(Tree, COREKIT_BASICUI_TEXT("TimeFrozen", "TIME FROZEN"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::TextDim, 0.2f)));
	CoreKitBasicUIBuild::AddH(Chips, Frozen, FMargin(0.f, 0.f, 12.f, 0.f));
	UBorder* Queued = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Lime), FMargin(10.f, 4.f), TEXT("QueuedChip"));
	QueuedChipText = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 700, CoreKitBasicUIColor::Soot, 0.2f));
	Queued->SetContent(QueuedChipText);
	CoreKitBasicUIBuild::AddH(Chips, Queued);
	QueuedChip = Queued;
	ChipRow = Chips;
	CoreKitBasicUIBuild::AddV(HeaderBox, Chips, FMargin(0.f), HAlign_Center);
	UHorizontalBox* DrawOne = CoreKitBasicUIBuild::Row(Tree, TEXT("DrawOne"));
	CoreKitBasicUIBuild::AddH(DrawOne, Label(Tree, COREKIT_BASICUI_TEXT("DrawWord", "DRAW "), CoreKitBasicUIBuild::WithShadow(CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(104.f), 700))));
	CoreKitBasicUIBuild::AddH(DrawOne, Label(Tree, COREKIT_BASICUI_TEXT("OneWord", "ONE"), CoreKitBasicUIBuild::WithShadow(CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(104.f), 700, CoreKitBasicUIColor::Magenta))));
	CoreKitBasicUIBuild::AddV(HeaderBox, DrawOne, FMargin(0.f, 6.f, 0.f, 0.f), HAlign_Center);
	DrawOneTitle = DrawOne;
	CoreKitBasicUIBuild::Place(RootCanvas, HeaderBox, FAnchors(0.5f, 0.f), FVector2D(0.f, 56.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f), 21);
	Header = HeaderBox;
	Chips->SetRenderOpacity(0.f);
	DrawOne->SetRenderOpacity(0.f);

	CardLayer = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Cards"));
	CardLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CoreKitBasicUIBuild::Fill(RootCanvas, CardLayer, FMargin(0.f), 10);

	// Footer: YOUR BUILD + mini hand + hint · keys + status plate.
	UHorizontalBox* FooterRow = CoreKitBasicUIBuild::Row(Tree, TEXT("Footer"));
	FooterRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* FooterSlot = RootCanvas->AddChildToCanvas(FooterRow);
	FooterSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	FooterSlot->SetAlignment(FVector2D(0.f, 1.f));
	FooterSlot->SetOffsets(FMargin(120.f, -44.f, 120.f, 80.f));
	FooterSlot->SetZOrder(15);
	CoreKitBasicUIBuild::AddH(FooterRow, Label(Tree, COREKIT_BASICUI_TEXT("YourBuild", "YOUR BUILD"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Steel, 0.2f)), FMargin(0.f, 0.f, 14.f, 0.f));
	Hand = CoreKitBasicUIBuild::Row(Tree, TEXT("Hand"));
	CoreKitBasicUIBuild::AddH(FooterRow, Hand, FMargin(0.f), VAlign_Bottom);
	CoreKitBasicUIBuild::AddH(FooterRow, Label(Tree, COREKIT_BASICUI_TEXT("Hint", "HOVER A CARD · IT TILTS TOWARD THE CURSOR"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(11.f), 500, CoreKitBasicUIColor::Steel, 0.14f)),
		FMargin(18.f, 0.f, 0.f, 0.f), VAlign_Center, HAlign_Left, 1.f);
	UHorizontalBox* Keys = CoreKitBasicUIBuild::Row(Tree, TEXT("DrawKeys"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UCoreKitBasicUIKeycap* Key = Tree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass());
		Key->SetKey(CardKey(Index));
		CoreKitBasicUIBuild::AddH(Keys, Key, FMargin(0.f, 0.f, 8.f, 0.f));
		FooterKeys.Add(Key);
	}
	CoreKitBasicUIBuild::AddH(Keys, Label(Tree, COREKIT_BASICUI_TEXT("DrawKey", "DRAW"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::TextDim, 0.14f)));
	CoreKitBasicUIBuild::AddH(FooterRow, Keys, FMargin(0.f, 0.f, 28.f, 0.f));
	StatusPlate = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Chamfer(18, CoreKitBasicUIColor::White), FMargin(34.f, 0.f), TEXT("StatusPlate"));
	StatusPlate->SetVerticalAlignment(VAlign_Center);
	StatusText = Label(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(28.f), 700, CoreKitBasicUIColor::Soot), TEXT("StatusText"));
	StatusPlate->SetContent(StatusText);
	CoreKitBasicUIBuild::AddH(FooterRow, CoreKitBasicUIBuild::Box(Tree, StatusPlate, 0.f, 64.f));
	FooterRow->SetRenderOpacity(0.f);
	Footer = FooterRow;
	SetStatus(false, FText::GetEmpty());
}

void UCoreKitBasicUICardDrawMenu::NativeConstruct()
{
	Super::NativeConstruct();
	StartOffer(false);
}

void UCoreKitBasicUICardDrawMenu::NativeDestruct()
{
	// Never leave the world frozen or the HUD dimmed behind a closed overlay.
	SetFreeze(0.f);
	if (UCoreKitBasicUIScreenRouter* Router = CoreKitBasicUIOverlay::FindRouter(this))
	{
		Router->SetHudOpacity(1.f, 0.25f);
	}
	Super::NativeDestruct();
}

void UCoreKitBasicUICardDrawMenu::StartOffer(bool bChained)
{
	UCoreKitBasicUIProgressionViewModel* VM = Progression();
	Offer = VM ? VM->GetOffer() : TArray<FCoreKitBasicUICardData>();
	DealtSerial = VM ? VM->GetOfferSerial() : INDEX_NONE;
	Flow.Reset();
	SinceCommit = -1.f;
	bUnfreezing = false;
	DealCuesPlayed = 0;
	BuildCards();
	RefreshHeader();
	RefreshHand();
	SetStatus(false, FText::GetEmpty());

	TitleRise.Snap(0.f);
	TitleScale.Snap(1.35f);
	TitleOpacity.Snap(0.f);
	IntroSlashScale.Snap(0.f);
	IntroSlashOpacity.Snap(0.f);
	DrawOneOpacity.Snap(0.f);
	if (bChained)
	{
		// Queued level: the world stays frozen; the title slams again and a fresh hand is dealt.
		Clock = TimeTitle - 0.05f;
		FiredSteps = StepXP | StepFlash;
		ChooseSlashOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit);
	}
	else
	{
		Clock = 0.f;
		FiredSteps = 0;
		FreezeTween.Snap(0.f);
		ChipsOpacity.Snap(0.f);
		ChooseSlashOpacity.Snap(0.f);
		FooterOpacity.Snap(0.f);
	}
}

FVector2D UCoreKitBasicUICardDrawMenu::SlotCenter(int32 Index) const
{
	const float Mid = (Offer.Num() - 1) * 0.5f;
	return FVector2D((Index - Mid) * (CardWidth + CardGap), CardTop + CardHeight * 0.5f);
}

void UCoreKitBasicUICardDrawMenu::BuildCards()
{
	if (!CardLayer)
	{
		return;
	}
	CardLayer->ClearChildren();
	Slots.Reset();
	KeepAlive.RemoveAll([](const TObjectPtr<UObject>& Object) { return Object && (Object->IsA<UCoreKitButton>() || Object->IsA<UCoreKitBasicUICardWidget>()); });
	const float Mid = (Offer.Num() - 1) * 0.5f;
	for (int32 Index = 0; Index < Offer.Num(); ++Index)
	{
		FCardSlot& CardSlot = Slots.AddDefaulted_GetRef();
		UCoreKitButton* Button = WidgetTree->ConstructWidget<UCoreKitButton>(UCoreKitButton::StaticClass());
		Button->SetBaseStyle(UCoreKitBasicUIStyle::ClearButtonStyle());
		Button->ButtonIndex = Index;
		Button->bPlaySounds = false;
		Button->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleCardActivated);
		Button->OnSelected.AddUniqueDynamic(this, &ThisClass::HandleCardSelected);
		UCoreKitBasicUICardWidget* Card = WidgetTree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass());
		Card->SetVariant(ECoreKitBasicUICardVariant::Full);
		Card->SetIdleBrightness(0.82f);
		Card->SetCard(Offer[Index]);
		Card->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->AddChild(Card);
		const FVector2D Center = SlotCenter(Index);
		CoreKitBasicUIBuild::Place(CardLayer, Button, FAnchors(0.5f, 0.f), FVector2D(Center.X - CardWidth * 0.5f, CardTop), FVector2D(CardWidth, CardHeight));
		Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		CardSlot.Button = Button;
		CardSlot.Card = Card;
		CardSlot.Base = (Index - Mid) * DealRotation;
		CardSlot.X.Snap(0.f);
		CardSlot.Y.Snap(640.f);
		CardSlot.Angle.Snap(CardSlot.Base);
		CardSlot.Scale.Snap(0.7f);
		CardSlot.Opacity.Snap(0.f);
		CoreKitBasicUIOverlay::Pose(Button, FVector2D(0.f, 640.f), 0.7f, CardSlot.Base, 0.f);
		KeepAlive.Add(Button);
		KeepAlive.Add(Card);
	}
	bKeysDirty = true;
	PadFocusIndex = INDEX_NONE;
	RefreshKeyHints();
}

void UCoreKitBasicUICardDrawMenu::RefreshHeader()
{
	const UCoreKitBasicUIProgressionViewModel* VM = Progression();
	const int32 Queued = VM ? FMath::Max(0, VM->GetQueuedDraws()) : 0;
	// The offer is for the oldest pending level: the component's level already includes the queued ones.
	const int32 OfferLevel = VM ? FMath::Max(1, VM->GetLevel() - Queued) : 1;
	const FText LevelText = FText::Format(COREKIT_BASICUI_TEXT("LevelN", "LEVEL {0}"), FText::FromString(CoreKitBasicUIOverlay::TwoDigits(OfferLevel)));
	if (IntroLevel)
	{
		IntroLevel->SetText(LevelText);
	}
	if (LevelChip)
	{
		LevelChip->SetText(LevelText);
	}
	if (QueuedChip)
	{
		QueuedChip->SetVisibility(Queued > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		QueuedChipText->SetText(FText::Format(COREKIT_BASICUI_TEXT("Queued", "+{0} DRAW QUEUED"), FText::AsNumber(Queued)));
	}
}

void UCoreKitBasicUICardDrawMenu::RefreshHand()
{
	const UCoreKitBasicUIBuildViewModel* VM = Build();
	HandSerial = VM ? VM->GetPickSerial() : INDEX_NONE;
	if (!Hand)
	{
		return;
	}
	Hand->ClearChildren();
	const TArray<FCoreKitBasicUICardData> Cards = VM ? VM->GetCards() : TArray<FCoreKitBasicUICardData>();
	const int32 Count = Cards.Num();
	const float Step = Count > 1 ? FMath::Min(4.f, 16.f / (Count - 1)) : 0.f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UCoreKitBasicUICardWidget* Mini = WidgetTree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass());
		Mini->SetVariant(ECoreKitBasicUICardVariant::Mini);
		Mini->SetCard(Cards[Index]);
		Mini->SetVisibility(ESlateVisibility::HitTestInvisible);
		const float Angle = (Index - (Count - 1) * 0.5f) * Step;
		CoreKitBasicUIBuild::SetTransform(Mini, FVector2D(0.f, FMath::Abs(Angle) * 0.5f), 1.f, Angle);
		CoreKitBasicUIBuild::AddH(Hand, Mini, FMargin(Index > 0 ? -8.f : 0.f, 0.f, 0.f, 0.f), VAlign_Bottom);
	}
}

void UCoreKitBasicUICardDrawMenu::RefreshKeyHints()
{
	const ULocalPlayer* Player = GetOwningLocalPlayer();
	const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	const bool bPad = Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad;
	if (bPad != bGamepadKeys || bKeysDirty)
	{
		bGamepadKeys = bPad;
		bKeysDirty = false;
		PadFocusIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Slots[Index].Card)
			{
				Slots[Index].Card->SetKeyHintKey(bPad ? FKey() : CardKey(Index));
			}
		}
		for (int32 Index = 0; Index < FooterKeys.Num(); ++Index)
		{
			FooterKeys[Index]->SetKey(bPad ? (Index == 0 ? EKeys::Gamepad_FaceButton_Bottom : FKey()) : CardKey(Index));
			FooterKeys[Index]->SetVisibility(bPad && Index > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}
	if (!bPad)
	{
		return;
	}
	// With a gamepad only the focused card shows the accept glyph.
	const int32 Focused = Slots.IndexOfByPredicate([](const FCardSlot& S) { return S.Button && S.Button->IsSelected(); });
	if (Focused != PadFocusIndex)
	{
		if (Slots.IsValidIndex(PadFocusIndex) && Slots[PadFocusIndex].Card)
		{
			Slots[PadFocusIndex].Card->SetKeyHintKey(FKey());
		}
		if (Slots.IsValidIndex(Focused) && Slots[Focused].Card)
		{
			Slots[Focused].Card->SetKeyHintKey(EKeys::Gamepad_FaceButton_Bottom);
		}
		PadFocusIndex = Focused;
	}
}

void UCoreKitBasicUICardDrawMenu::SetStatus(bool bDrawn, const FText& CardName)
{
	if (!StatusPlate)
	{
		return;
	}
	StatusPlate->SetBrushColor(bDrawn ? CoreKitBasicUIColor::Mint : CoreKitBasicUIColor::White);
	StatusText->SetText(bDrawn ? FText::Format(COREKIT_BASICUI_TEXT("Drawn", "{0} DRAWN · RESUMING"), CardName.ToUpper()) : COREKIT_BASICUI_TEXT("Click", "CLICK A CARD TO DRAW IT"));
}

void UCoreKitBasicUICardDrawMenu::SetFreeze(float Alpha)
{
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	if (UCoreKitBasicUIScreenEffectsComponent* Effects = UCoreKitBasicUIScreenEffectsComponent::Get(this))
	{
		Effects->SetFreezeAmount(A);
	}
	if (Blur)
	{
		Blur->SetBlurStrength(10.f * A);
		Blur->SetVisibility(A > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (VeilGroup)
	{
		VeilGroup->SetRenderOpacity(A);
	}
}

UWidget* UCoreKitBasicUICardDrawMenu::GetInitialFocus_Implementation() const
{
	return Slots.Num() > 0 ? Slots[0].Button.Get() : nullptr;
}

bool UCoreKitBasicUICardDrawMenu::HandleReturn_Implementation()
{
	// There is no way back from a draw: swallow Return so it does not reach the game.
	return true;
}

bool UCoreKitBasicUICardDrawMenu::HandleMenuKey_Implementation(FKey Key)
{
	int32 Index = INDEX_NONE;
	if (Key == EKeys::One || Key == EKeys::NumPadOne) Index = 0;
	else if (Key == EKeys::Two || Key == EKeys::NumPadTwo) Index = 1;
	else if (Key == EKeys::Three || Key == EKeys::NumPadThree) Index = 2;
	if (Index == INDEX_NONE)
	{
		return false;
	}
	ChooseCard(Index);
	return true;
}

FReply UCoreKitBasicUICardDrawMenu::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	if (Key == EKeys::Gamepad_RightX)
	{
		StickTilt.X = FMath::Abs(InAnalogEvent.GetAnalogValue()) > 0.15f ? InAnalogEvent.GetAnalogValue() : 0.f;
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_RightY)
	{
		StickTilt.Y = FMath::Abs(InAnalogEvent.GetAnalogValue()) > 0.15f ? -InAnalogEvent.GetAnalogValue() : 0.f;
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

void UCoreKitBasicUICardDrawMenu::HandleCardActivated(UCoreKitButton* Button)
{
	if (Button)
	{
		ChooseCard(Button->ButtonIndex);
	}
}

void UCoreKitBasicUICardDrawMenu::HandleCardSelected(UCoreKitButton* Button)
{
	if (Button && Flow.State == FCoreKitBasicUICardDrawFlow::EState::Choose && HoveredCue != Button->ButtonIndex)
	{
		HoveredCue = Button->ButtonIndex;
		PlayCue(TEXT("Cards.Hover"));
	}
}

bool UCoreKitBasicUICardDrawMenu::ChooseCard(int32 Index)
{
	if (Flow.IsLocked())
	{
		if (Flow.State == FCoreKitBasicUICardDrawFlow::EState::Intro || Flow.State == FCoreKitBasicUICardDrawFlow::EState::Choose)
		{
			PlayCue(TEXT("UI.Error"));
		}
		return false;
	}
	if (!Flow.TryPick(Index, Offer.Num()))
	{
		return false;
	}
	BeginPick(Index);
	return true;
}

void UCoreKitBasicUICardDrawMenu::BeginPick(int32 Index)
{
	PlayCue(TEXT("Cards.Pick"));
	const float Height = RootCanvas ? FMath::Max(1.f, static_cast<float>(RootCanvas->GetCachedGeometry().GetLocalSize().Y)) : CoreKitBasicUIOverlay::Reference.Y;
	const float ViewHeight = Height > 1.f ? Height : CoreKitBasicUIOverlay::Reference.Y;
	// Target: the build hand at the bottom centre of the HUD (left 660, bottom 54, 600 x 88), right end.
	const FVector2D Target(140.f, ViewHeight - 54.f - 36.f);
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		FCardSlot& CardSlot = Slots[SlotIndex];
		if (SlotIndex == Index)
		{
			CardSlot.Card->SetState(ECoreKitBasicUICardState::Selected);
			CardSlot.Card->SetTilt(FVector2D::ZeroVector);
			CardSlot.Card->SetFocusOutline(false);
			const FVector2D Center = SlotCenter(SlotIndex);
			CardSlot.X.Start(Target.X - Center.X, ECoreKitBasicUIMotion::Deal, 0.5f, PickFlyStart);
			CardSlot.Y.Start(Target.Y - Center.Y, ECoreKitBasicUIMotion::Deal, 0.5f, PickFlyStart);
			CardSlot.Angle.Start(12.f, ECoreKitBasicUIMotion::Deal, 0.5f, PickFlyStart);
			CardSlot.Scale.Start(0.17f, ECoreKitBasicUIMotion::Deal, 0.5f, PickFlyStart);
			CardSlot.Opacity.Start(1.f, 0.f, ECoreKitBasicUIMotion::Linear, 0.08f, PickArrive);
		}
		else
		{
			const float Side = SlotIndex < Index ? -1.f : 1.f;
			CardSlot.Card->SetState(ECoreKitBasicUICardState::Dimmed);
			CardSlot.Card->SetTilt(FVector2D::ZeroVector);
			CardSlot.Card->SetFocusOutline(false);
			CardSlot.X.Start(Side * 40.f, ECoreKitBasicUIMotion::Exit, 0.3f);
			CardSlot.Y.Start(320.f, ECoreKitBasicUIMotion::Exit, 0.3f);
			CardSlot.Angle.Start(Side * 14.f, ECoreKitBasicUIMotion::Exit, 0.3f);
			CardSlot.Scale.Start(0.9f, ECoreKitBasicUIMotion::Exit, 0.3f);
			CardSlot.Opacity.Start(0.f, ECoreKitBasicUIMotion::Exit, 0.3f);
		}
		CardSlot.Button->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	SetStatus(true, Offer.IsValidIndex(Index) ? Offer[Index].Name : FText::GetEmpty());
}

void UCoreKitBasicUICardDrawMenu::Commit()
{
	UCoreKitBasicUIProgressionViewModel* VM = Progression();
	const bool bAccepted = VM && VM->RequestChooseCard(Flow.Picked);
	if (!bAccepted)
	{
		// Gameplay rejected the pick (stale offer): put the hand back on the table and lock again.
		PlayCue(TEXT("UI.Error"));
		Flow.RevertPick();
		for (FCardSlot& CardSlot : Slots)
		{
			CardSlot.Card->SetState(ECoreKitBasicUICardState::Idle);
			CardSlot.X.Start(0.f, ECoreKitBasicUIMotion::Deal, 0.3f);
			CardSlot.Y.Start(0.f, ECoreKitBasicUIMotion::Deal, 0.3f);
			CardSlot.Angle.Start(0.f, ECoreKitBasicUIMotion::Deal, 0.3f);
			CardSlot.Scale.Start(1.f, ECoreKitBasicUIMotion::Deal, 0.3f);
			CardSlot.Opacity.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
			CardSlot.Button->SetVisibility(ESlateVisibility::Visible);
		}
		SetStatus(false, FText::GetEmpty());
		return;
	}
	SinceCommit = 0.f;
	if (VM->GetbOfferOpen() && VM->GetOfferSerial() != DealtSerial && VM->GetOffer().Num() > 0)
	{
		StartOffer(true);
	}
	else
	{
		BeginUnfreeze();
	}
}

void UCoreKitBasicUICardDrawMenu::BeginUnfreeze()
{
	bUnfreezing = true;
	FreezeTween.Start(0.f, ECoreKitBasicUIMotion::Settle, UnfreezeSeconds);
	ChipsOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit, UnfreezeSeconds);
	DrawOneOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit, UnfreezeSeconds);
	ChooseSlashOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit, UnfreezeSeconds);
	FooterOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit, UnfreezeSeconds, 0.1f);
	if (UCoreKitBasicUIScreenRouter* Router = CoreKitBasicUIOverlay::FindRouter(this))
	{
		Router->SetHudOpacity(1.f, UnfreezeSeconds);
	}
}

void UCoreKitBasicUICardDrawMenu::UpdateSequence(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	auto Fire = [this](uint8 Step, float Time)
	{
		if (!(FiredSteps & Step) && Clock >= Time)
		{
			FiredSteps |= Step;
			return true;
		}
		return false;
	};

	if (Fire(StepXP, TimeXPFill))
	{
		// The HUD's XP bar fills with its glint (HUD event #10); the overlay adds the riser.
		PlayCue(TEXT("HUD.LevelUp"));
	}
	if (Fire(StepFlash, TimeFlash))
	{
		FlashClock = 0.f;
	}
	if (Fire(StepTitle, TimeTitle))
	{
		PlayCue(TEXT("HUD.LevelUpHit"));
		TitleScale.Start(1.35f, 1.f, ECoreKitBasicUIMotion::Slam, 0.38f);
		TitleOpacity.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
		TitleRise.Snap(0.f);
		IntroSlashScale.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle, 0.26f);
		IntroSlashOpacity.Snap(1.f);
		FreezeTween.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.3f);
		ChipsOpacity.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
		// HUD to 0.25 / hand to 0.35: the HUD dims itself while the offer is open (UCoreKitBasicUIHudRoot).
	}
	if (Fire(StepDeal, TimeDeal))
	{
		const float Stagger = 0.07f;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			FCardSlot& CardSlot = Slots[Index];
			const float Delay = Index * Stagger;
			CardSlot.X.Start(0.f, ECoreKitBasicUIMotion::Deal, 0.52f, Delay);
			CardSlot.Y.Start(0.f, ECoreKitBasicUIMotion::Deal, 0.52f, Delay);
			CardSlot.Angle.Start(0.f, ECoreKitBasicUIMotion::Deal, 0.52f, Delay);
			CardSlot.Scale.Start(1.f, ECoreKitBasicUIMotion::Deal, 0.52f, Delay);
			CardSlot.Opacity.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle, 0.2f, Delay);
		}
		TitleRise.Start(0.f, -300.f, ECoreKitBasicUIMotion::Deal, 0.52f);
		TitleScale.Start(0.6f, ECoreKitBasicUIMotion::Deal, 0.52f);
		TitleOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit, 0.3f, 0.2f);
		DrawOneOpacity.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle, 0.3f, 0.25f);
		IntroSlashOpacity.Start(0.f, ECoreKitBasicUIMotion::Exit, 0.3f);
		ChooseSlashOpacity.Start(0.5f, ECoreKitBasicUIMotion::Settle, 0.3f, 0.2f);
		FooterOpacity.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.3f, 0.3f);
	}
	// One deal cue per card, on its stagger.
	while (DealCuesPlayed < Slots.Num() && (FiredSteps & StepDeal) && Clock >= TimeDeal + DealCuesPlayed * 0.07f)
	{
		++DealCuesPlayed;
		PlayCue(TEXT("Cards.Deal"));
	}
	if (Fire(StepChoose, TimeChoose))
	{
		Flow.BeginChoose();
		const int32 Focused = Slots.IndexOfByPredicate([](const FCardSlot& S) { return S.Button && S.Button->IsSelected(); });
		HoveredCue = Focused;
		if (Slots.Num() > 0 && Focused == INDEX_NONE)
		{
			HoveredCue = 0;
			Slots[0].Button->Select();
		}
	}
}

void UCoreKitBasicUICardDrawMenu::UpdateTilt()
{
	const ULocalPlayer* Player = GetOwningLocalPlayer();
	const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	const bool bPad = Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad;
	const bool bChoosing = Flow.State == FCoreKitBasicUICardDrawFlow::EState::Choose;
	for (FCardSlot& CardSlot : Slots)
	{
		if (!CardSlot.Button || !CardSlot.Card || Flow.State == FCoreKitBasicUICardDrawFlow::EState::Picking || Flow.State == FCoreKitBasicUICardDrawFlow::EState::Committed)
		{
			continue;
		}
		const bool bSelected = bChoosing && CardSlot.Button->IsSelected();
		CardSlot.Card->SetState(bSelected ? ECoreKitBasicUICardState::Hover : ECoreKitBasicUICardState::Idle);
		CardSlot.Card->SetFocusOutline(bSelected && bPad);
		FVector2D Target = FVector2D::ZeroVector;
		if (bSelected && !CoreKitBasicUIMotion::IsReducedMotion())
		{
			if (bPad)
			{
				Target = StickTilt;
			}
			else if (CardSlot.Button->IsHovered() && FSlateApplication::IsInitialized())
			{
				const FGeometry& Geometry = CardSlot.Button->GetCachedGeometry();
				const FVector2D Size = Geometry.GetLocalSize();
				if (Size.X > 1.f && Size.Y > 1.f)
				{
					const FVector2D Local = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
					Target = FVector2D(FMath::Clamp(Local.X / Size.X * 2.f - 1.f, -1.f, 1.f), FMath::Clamp(Local.Y / Size.Y * 2.f - 1.f, -1.f, 1.f));
				}
			}
		}
		CardSlot.Card->SetTilt(Target);
	}
}

void UCoreKitBasicUICardDrawMenu::UpdateCards(float DeltaSeconds)
{
	for (FCardSlot& CardSlot : Slots)
	{
		CardSlot.X.Tick(DeltaSeconds);
		CardSlot.Y.Tick(DeltaSeconds);
		CardSlot.Angle.Tick(DeltaSeconds);
		CardSlot.Scale.Tick(DeltaSeconds);
		CardSlot.Opacity.Tick(DeltaSeconds);
		CoreKitBasicUIOverlay::Pose(CardSlot.Button, FVector2D(CardSlot.X.Value, CardSlot.Y.Value), CardSlot.Scale.Value, CardSlot.Angle.Value, CardSlot.Opacity.Value);
	}
}

void UCoreKitBasicUICardDrawMenu::TickMotion(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	UCoreKitBasicUIProgressionViewModel* VM = Progression();

	// The offer vanished before a pick (restart, quit): nothing to choose any more.
	const bool bBeforeCommit = Flow.State == FCoreKitBasicUICardDrawFlow::EState::Intro || Flow.State == FCoreKitBasicUICardDrawFlow::EState::Choose;
	if (bBeforeCommit && (!VM || !VM->GetbOfferOpen() || VM->GetOffer().Num() == 0))
	{
		bFinished = true;
		CloseMenu();
		return;
	}
	// A different offer replaced the one on the table (or a queued level arrived while unfreezing).
	if (VM && VM->GetbOfferOpen() && VM->GetOffer().Num() > 0 && VM->GetOfferSerial() != DealtSerial
		&& (bBeforeCommit || Flow.State == FCoreKitBasicUICardDrawFlow::EState::Committed))
	{
		if (bUnfreezing)
		{
			FreezeTween.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
			ChipsOpacity.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
			FooterOpacity.Start(1.f, ECoreKitBasicUIMotion::Settle, 0.2f);
		}
		StartOffer(true);
	}

	UpdateSequence(DeltaSeconds);
	Flow.Tick(DeltaSeconds);
	if (Flow.ConsumeCommit(PickArrive))
	{
		Commit();
	}
	if (SinceCommit >= 0.f)
	{
		SinceCommit += DeltaSeconds;
	}

	// Freeze, flash, title, slashes, chips, footer.
	FreezeTween.Tick(DeltaSeconds);
	SetFreeze(FreezeTween.Value);
	if (FlashClock >= 0.f)
	{
		FlashClock += DeltaSeconds;
		const float T = FlashClock / 0.28f;
		const float A = T < 0.2f ? 0.75f * (T / 0.2f) : 0.75f * FMath::Max(0.f, 1.f - (T - 0.2f) / 0.8f);
		Flash->SetRenderOpacity(A);
		if (T >= 1.f)
		{
			FlashClock = -1.f;
			Flash->SetRenderOpacity(0.f);
		}
	}
	for (FCoreKitBasicUITween* Tween : { &TitleScale, &TitleOpacity, &TitleRise, &IntroSlashScale, &IntroSlashOpacity, &ChooseSlashOpacity, &ChipsOpacity, &DrawOneOpacity, &FooterOpacity })
	{
		Tween->Tick(DeltaSeconds);
	}
	CoreKitBasicUIOverlay::Pose(IntroTitle, FVector2D(0.f, TitleRise.Value), TitleScale.Value, 0.f, TitleOpacity.Value);
	if (IntroSlash)
	{
		FWidgetTransform SlashTransform;
		SlashTransform.Scale = FVector2D(FMath::Max(0.001f, IntroSlashScale.Value), 1.f);
		SlashTransform.Angle = -4.f;
		IntroSlash->SetRenderTransform(SlashTransform);
		IntroSlash->SetRenderOpacity(IntroSlashOpacity.Value);
	}
	ChooseSlash->SetRenderOpacity(ChooseSlashOpacity.Value);
	ChipRow->SetRenderOpacity(ChipsOpacity.Value);
	DrawOneTitle->SetRenderOpacity(DrawOneOpacity.Value);
	Footer->SetRenderOpacity(FooterOpacity.Value);

	UpdateCards(DeltaSeconds);
	UpdateTilt();
	RefreshKeyHints();
	if (const UCoreKitBasicUIBuildViewModel* BuildVM = Build(); BuildVM && BuildVM->GetPickSerial() != HandSerial)
	{
		RefreshHand();
	}

	if (bUnfreezing && !FreezeTween.IsActive() && SinceCommit >= UnfreezeSeconds + 0.05f)
	{
		bFinished = true;
		SetFreeze(0.f);
		CloseMenu();
	}
}

#undef LOCTEXT_NAMESPACE
