#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUICardWidget"

namespace
{
	const FVector2D FullSize(360.f, 540.f);
	const FVector2D FullScene(352.f, 250.f);
	/** Scale of the small crops relative to the 352 x 250 scene (06-Cards: ~0.36). */
	constexpr float CropScale = 0.36f;

	FText DefaultSuitLabel(ECoreKitBasicUICardSuit Suit)
	{
		switch (Suit)
		{
		case ECoreKitBasicUICardSuit::Sight: return COREKIT_BASICUI_TEXT("SuitSight", "SIGHT · RIFLE");
		case ECoreKitBasicUICardSuit::Edge: return COREKIT_BASICUI_TEXT("SuitEdge", "EDGE · KATANA");
		case ECoreKitBasicUICardSuit::Vector: return COREKIT_BASICUI_TEXT("SuitVector", "VECTOR · DASH");
		default: return COREKIT_BASICUI_TEXT("SuitPlate", "PLATE · ARMOR");
		}
	}

	UImage* PlaceImage(UWidgetTree* Tree, UCanvasPanel* Canvas, const FSlateBrush& Brush, FVector2D Pos, FVector2D Size, float Alpha = 1.f, int32 Z = 0)
	{
		UImage* Image = CoreKitBasicUIBuild::Image(Tree, Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Image->SetRenderOpacity(Alpha);
		CoreKitBasicUIBuild::Place(Canvas, Image, FAnchors(0.f, 0.f), Pos, Size, FVector2D::ZeroVector, Z);
		return Image;
	}

	/** Vertical alpha ramp; bDown = transparent at the top, opaque at the bottom (T_UI_GradV). */
	UImage* PlaceRamp(UWidgetTree* Tree, UCanvasPanel* Canvas, const FLinearColor& Color, FVector2D Pos, FVector2D Size, bool bDown)
	{
		UImage* Image = PlaceImage(Tree, Canvas, UCoreKitBasicUIStyle::Image(TEXT("T_UI_GradV"), Color), Pos, Size);
		if (!bDown)
		{
			FWidgetTransform Flip;
			Flip.Scale = FVector2D(1.f, -1.f);
			Image->SetRenderTransform(Flip);
		}
		return Image;
	}
}

FVector2D UCoreKitBasicUICardWidget::VariantSize(ECoreKitBasicUICardVariant InVariant, float InThumbSize)
{
	switch (InVariant)
	{
	case ECoreKitBasicUICardVariant::Mini: return FVector2D(52.f, 72.f);
	case ECoreKitBasicUICardVariant::Victory: return FVector2D(76.f, 100.f);
	case ECoreKitBasicUICardVariant::Defeat: return FVector2D(62.f, 82.f);
	case ECoreKitBasicUICardVariant::Thumb: return FVector2D(InThumbSize, InThumbSize);
	default: return FullSize;
	}
}

void UCoreKitBasicUICardWidget::SetVariant(ECoreKitBasicUICardVariant InVariant, float InThumbSize)
{
	const bool bChanged = InVariant != Variant || !FMath::IsNearlyEqual(InThumbSize, ThumbSize);
	Variant = InVariant;
	ThumbSize = InThumbSize;
	if (bChanged)
	{
		Rebuild();
	}
}

void UCoreKitBasicUICardWidget::SetCard(const FCoreKitBasicUICardData& InCard)
{
	const bool bSameLook = Card.Id == InCard.Id && Card.Copies == InCard.Copies && Card.Footer.EqualTo(InCard.Footer)
		&& Card.ArtScene == InCard.ArtScene && Card.ArtEffect == InCard.ArtEffect && Card.Name.EqualTo(InCard.Name);
	Card = InCard;
	if (!bSameLook)
	{
		Rebuild();
	}
}

void UCoreKitBasicUICardWidget::SetKeyHint(const FText& InKey)
{
	KeyHint = InKey;
	KeyHintKey = FKey();
	ApplyKeyHint();
}

void UCoreKitBasicUICardWidget::SetKeyHintKey(FKey InKey)
{
	KeyHintKey = InKey;
	KeyHint = FText::GetEmpty();
	ApplyKeyHint();
}

void UCoreKitBasicUICardWidget::SetFooter(const FText& InFooter)
{
	FooterOverride = InFooter;
	if (FooterText)
	{
		FooterText->SetText(!FooterOverride.IsEmpty() ? FooterOverride : !Card.Footer.IsEmpty() ? Card.Footer
			: (Card.bSpecial ? COREKIT_BASICUI_TEXT("FootSpecial", "UNIQUE  ·  LEAVES THE POOL") : COREKIT_BASICUI_TEXT("FootStat", "STACKS  ·  EVERY COPY ADDS")));
	}
}

void UCoreKitBasicUICardWidget::SetState(ECoreKitBasicUICardState InState)
{
	if (State == InState)
	{
		return;
	}
	State = InState;
	ApplyStateLook();
}

void UCoreKitBasicUICardWidget::SetTilt(FVector2D InTilt)
{
	Tilt = FVector2D(FMath::Clamp(InTilt.X, -1.f, 1.f), FMath::Clamp(InTilt.Y, -1.f, 1.f));
}

void UCoreKitBasicUICardWidget::SetNew(bool bInNew)
{
	bNew = bInNew;
	if (bNew)
	{
		GlowAlpha.Start(0.9f, 0.f, ECoreKitBasicUIMotion::Linear, 4.f);
		NewPop.Start(0.f, 1.f, ECoreKitBasicUIMotion::Slam, 0.38f);
	}
	else
	{
		GlowAlpha.Snap(0.f);
		NewPop.Snap(0.f);
	}
	if (NewTag)
	{
		NewTag->SetVisibility(bNew ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCoreKitBasicUICardWidget::SetFocusOutline(bool bInVisible)
{
	if (bFocusOutline == bInVisible)
	{
		return;
	}
	bFocusOutline = bInVisible;
	for (UImage* Edge : FocusEdges)
	{
		Edge->SetVisibility(bFocusOutline ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

FLinearColor UCoreKitBasicUICardWidget::Accent() const
{
	const FLinearColor Color = Card.Accent();
	return Variant == ECoreKitBasicUICardVariant::Defeat && Color.Equals(CoreKitBasicUIColor::Magenta) ? CoreKitBasicUIColor::Red : Color;
}

FLinearColor UCoreKitBasicUICardWidget::FrameColor() const
{
	const FLinearColor Suit = UCoreKitBasicUIStyle::SuitColor(Card.Suit);
	if (Suit.Equals(CoreKitBasicUIColor::Mint))
	{
		return CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Mint, 0.55f);
	}
	if (Suit.Equals(CoreKitBasicUIColor::Magenta))
	{
		return CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.6f);
	}
	return CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::White, 0.55f);
}

void UCoreKitBasicUICardWidget::BuildTree()
{
	const FVector2D Size = VariantSize(Variant, ThumbSize);
	USizeBox* SizeRoot = CoreKitBasicUIBuild::Box(WidgetTree, nullptr, Size.X, Size.Y, TEXT("CardSize"));
	Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SizeRoot->AddChild(Root);
	WidgetTree->RootWidget = SizeRoot;
	bBuilt = true;
	Rebuild();
}

void UCoreKitBasicUICardWidget::Rebuild()
{
	if (!bBuilt || !Root)
	{
		return;
	}
	const FVector2D Size = VariantSize(Variant, ThumbSize);
	if (USizeBox* SizeRoot = Cast<USizeBox>(WidgetTree->RootWidget))
	{
		SizeRoot->SetWidthOverride(Size.X);
		SizeRoot->SetHeightOverride(Size.Y);
	}
	Root->ClearChildren();
	Glow = FrameSolid = FrameTape = SceneLayer = Shine = nullptr;
	SceneGroup = EffectGroup = NameText = NewTag = TextColumn = nullptr;
	EffectBox = nullptr;
	Keycap = nullptr;
	FooterText = nullptr;
	FocusEdges.Reset();
	bHasArt = false;

	if (Variant == ECoreKitBasicUICardVariant::Full)
	{
		BuildFull();
	}
	else
	{
		BuildSmall();
	}
	ApplyKeyHint();
	ApplyStateLook();
	Lift.Snap(Lift.To);
	Scale.Snap(Scale.To);
	Brightness.Snap(Brightness.To);
	SetNew(bNew);
	ApplyTilt();
}

void UCoreKitBasicUICardWidget::BuildArt(UCanvasPanel* Target, FVector2D SceneSize, bool bCrop)
{
	UWidgetTree* Tree = WidgetTree;
	const FLinearColor AccentColor = Accent();
	const FLinearColor SuitColor = Variant == ECoreKitBasicUICardVariant::Defeat && UCoreKitBasicUIStyle::SuitColor(Card.Suit).Equals(CoreKitBasicUIColor::Magenta)
		? CoreKitBasicUIColor::Red : UCoreKitBasicUIStyle::SuitColor(Card.Suit);
	PlaceImage(Tree, Target, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot), FVector2D::ZeroVector, SceneSize);

	UTexture2D* SceneTexture = CoreKitBasicUIOverlay::LoadCardArt(Card.ArtScene);
	UTexture2D* EffectTexture = CoreKitBasicUIOverlay::LoadCardArt(Card.ArtEffect);
	bHasArt = SceneTexture != nullptr;
	const FVector2D Focus(FMath::Clamp(Card.ArtFocus.X, 0.0, 1.0), FMath::Clamp(Card.ArtFocus.Y, 0.0, 1.0));
	const float K = FMath::Max3(CropScale, static_cast<float>(SceneSize.X / FullScene.X), static_cast<float>(SceneSize.Y / FullScene.Y));
	const FBox2f Crop = CoreKitBasicUIOverlay::CropUV(Focus, SceneSize / K, FullScene);

	auto ArtBrush = [&](UTexture2D* Texture, FVector2D DrawSize)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = DrawSize;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		if (bCrop)
		{
			Brush.SetUVRegion(Crop);
		}
		return Brush;
	};

	// Scene layer (back). Full: fixed 1.06 overscale around the focus so the parallax never shows an edge.
	const FVector2D FocusPx = bCrop ? SceneSize * 0.5f : Focus * SceneSize;
	const float Over = bCrop ? 1.f : SceneOverscale;
	const FVector2D LayerSize = SceneSize * Over;
	const FVector2D LayerPos = FocusPx - FocusPx * Over;
	SceneBasePos = LayerPos;
	if (SceneTexture)
	{
		SceneLayer = PlaceImage(Tree, Target, ArtBrush(SceneTexture, LayerSize), LayerPos, LayerSize);
		SceneGroup = SceneLayer;
	}
	else
	{
		// Placeholder "graded crop": suit glow over a dark plate, a faint ring for depth.
		UCanvasPanel* Plate = CoreKitBasicUIBuild::Canvas(Tree, TEXT("ScenePlaceholder"));
		Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::Place(Target, Plate, FAnchors(0.f, 0.f), LayerPos, LayerSize);
		const FVector2D Local = FocusPx - LayerPos;
		PlaceImage(Tree, Plate, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Hull), FVector2D::ZeroVector, LayerSize, 0.85f);
		const FVector2D GlowSize = bCrop ? SceneSize * 1.5f : FVector2D(LayerSize.X * 0.95f, LayerSize.Y * 1.25f);
		PlaceImage(Tree, Plate, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(AccentColor, 0.42f)), Local - GlowSize * 0.5f, GlowSize);
		if (!bCrop)
		{
			const float Ring = 190.f;
			PlaceImage(Tree, Plate, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Ring2"), CoreKitBasicUIColor::WithAlpha(AccentColor, 0.22f)), Local - FVector2D(Ring * 0.5f), FVector2D(Ring));
			PlaceImage(Tree, Plate, UCoreKitBasicUIStyle::Scanlines(0.05f), FVector2D::ZeroVector, LayerSize);
		}
		SceneGroup = Plate;
	}

	// Radial tint of the suit colour at the focal point (full card only).
	if (!bCrop)
	{
		const float TintAlpha = AccentColor.Equals(CoreKitBasicUIColor::White) ? 0.10f : 0.16f;
		const FVector2D TintSize(480.f, 340.f);
		PlaceImage(Tree, Target, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(AccentColor, TintAlpha)), FocusPx - TintSize * 0.5f, TintSize);
	}

	// Effect layer (front).
	if (EffectTexture)
	{
		EffectGroup = PlaceImage(Tree, Target, ArtBrush(EffectTexture, SceneSize), FVector2D::ZeroVector, SceneSize);
	}
	else if (!SceneTexture)
	{
		const float GlyphSize = bCrop ? FMath::Min(SceneSize.X, SceneSize.Y) * 0.5f : 104.f;
		UCanvasPanel* Effect = CoreKitBasicUIBuild::Canvas(Tree, TEXT("EffectPlaceholder"));
		Effect->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::Place(Target, Effect, FAnchors(0.f, 0.f), FVector2D::ZeroVector, SceneSize);
		if (!bCrop)
		{
			const FVector2D Halo(220.f, 220.f);
			PlaceImage(Tree, Effect, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(SuitColor, 0.35f)), FocusPx - Halo * 0.5f, Halo);
		}
		PlaceImage(Tree, Effect, UCoreKitBasicUIStyle::Image(UCoreKitBasicUIStyle::SuitIcon(Card.Suit), SuitColor, FVector2D(GlyphSize)), FocusPx - FVector2D(GlyphSize * 0.5f), FVector2D(GlyphSize), 0.95f);
		EffectGroup = Effect;
	}

	if (!bCrop)
	{
		// Vertical grade: Soot .72 -> 0 at 24 %, clear to 58 %, Hull .86 at 88 %, Hull at 100 %; subtle scanlines.
		const float H = SceneSize.Y;
		PlaceRamp(Tree, Target, CoreKitBasicUIColor::SootA(0.72f), FVector2D(0.f, 0.f), FVector2D(SceneSize.X, H * 0.24f), false);
		PlaceRamp(Tree, Target, CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Hull, 0.86f), FVector2D(0.f, H * 0.58f), FVector2D(SceneSize.X, H * 0.30f), true);
		PlaceImage(Tree, Target, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Hull, 0.86f)), FVector2D(0.f, H * 0.88f), FVector2D(SceneSize.X, H * 0.12f + 1.f));
		PlaceRamp(Tree, Target, CoreKitBasicUIColor::Hull, FVector2D(0.f, H * 0.88f), FVector2D(SceneSize.X, H * 0.12f + 1.f), true);
		PlaceImage(Tree, Target, UCoreKitBasicUIStyle::Scanlines(0.035f), FVector2D::ZeroVector, SceneSize);
	}
	else if (Variant != ECoreKitBasicUICardVariant::Thumb)
	{
		const float RampHeight = FMath::Min(24.f, SceneSize.Y * 0.4f);
		PlaceRamp(Tree, Target, CoreKitBasicUIColor::SootA(0.92f), FVector2D(0.f, SceneSize.Y - RampHeight), FVector2D(SceneSize.X, RampHeight), true);
	}
}

int32 UCoreKitBasicUICardWidget::BuildEffectText(UWrapBox* Box, const FLinearColor& AccentColor, float FontPx, int32 MaxWords)
{
	Box->ClearChildren();
	FString Pre = Card.EffectPre.ToString();
	FString Key = Card.EffectKey.ToString();
	FString Post = Card.EffectPost.ToString();
	if (Pre.IsEmpty() && Key.IsEmpty() && Post.IsEmpty())
	{
		Pre = Card.SummedEffect.ToString();
	}
	const TArray<CoreKitBasicUIOverlay::FEffectWord> Words = CoreKitBasicUIOverlay::SplitEffect(Pre, Key, Post);
	const int32 Count = MaxWords >= 0 ? FMath::Min(MaxWords, Words.Num()) : Words.Num();
	const bool bEllipsis = Count < Words.Num();
	for (int32 WordIndex = 0; WordIndex < Count; ++WordIndex)
	{
		UHorizontalBox* Word = CoreKitBasicUIBuild::Row(WidgetTree);
		const CoreKitBasicUIOverlay::FEffectWord& Runs = Words[WordIndex];
		for (int32 RunIndex = 0; RunIndex < Runs.Num(); ++RunIndex)
		{
			FString Text = Runs[RunIndex].Text;
			const bool bLastRun = RunIndex == Runs.Num() - 1;
			if (bLastRun && WordIndex == Count - 1 && bEllipsis)
			{
				Text.AppendChar(static_cast<TCHAR>(0x2026));
			}
			else if (bLastRun && WordIndex < Count - 1)
			{
				Text.AppendChar(TEXT(' '));
			}
			const CoreKitBasicUIBuild::FTextStyle Style = Runs[RunIndex].bKey
				? CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(FontPx), 700, AccentColor)
				: CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(FontPx), 400, CoreKitBasicUIColor::TextDim);
			// The display face has a deeper descent box than the body face: drop the accent run onto the body baseline.
			const FMargin RunPadding = Runs[RunIndex].bKey ? FMargin(0.f, 0.f, 0.f, -0.3f * FontPx) : FMargin(0.f);
			CoreKitBasicUIBuild::AddH(Word, CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(Text), Style), RunPadding, VAlign_Bottom);
		}
		// Bottom-align every word: the accent word uses the display face, whose line box is shorter than the body face,
		// so top alignment lifted it above the line.
		if (UWrapBoxSlot* WordSlot = Box->AddChildToWrapBox(Word))
		{
			WordSlot->SetVerticalAlignment(VAlign_Bottom);
		}
	}
	return Words.Num();
}

void UCoreKitBasicUICardWidget::FitText(float MaxHeight)
{
	UTextBlock* Name = Cast<UTextBlock>(NameText.Get());
	if (!TextColumn || !EffectBox || !Name || !FSlateApplication::IsInitialized())
	{
		return;
	}
	auto Height = [this]()
	{
		const TSharedRef<SWidget> Slate = TextColumn->TakeWidget();
		Slate->SlatePrepass(1.f);
		return static_cast<float>(Slate->GetDesiredSize().Y);
	};
	if (Height() <= MaxHeight)
	{
		return;
	}
	// One step smaller: name 40 -> 34 px, effect 22 -> 19 px.
	const FLinearColor AccentColor = Accent();
	Name->SetFont(UCoreKitBasicUIStyle::Font(ECoreKitBasicUIFontFamily::Display, 700, CoreKitBasicUIBuild::Px(34.f)));
	const int32 Words = BuildEffectText(EffectBox, AccentColor, 19.f);
	if (Height() <= MaxHeight)
	{
		return;
	}
	// Last resort: drop words from the end and close with an ellipsis.
	for (int32 Count = Words - 1; Count >= 1; --Count)
	{
		BuildEffectText(EffectBox, AccentColor, 19.f, Count);
		if (Height() <= MaxHeight)
		{
			return;
		}
	}
}

void UCoreKitBasicUICardWidget::BuildFull()
{
	UWidgetTree* Tree = WidgetTree;
	const FLinearColor AccentColor = Accent();
	const FLinearColor SuitColor = UCoreKitBasicUIStyle::SuitColor(Card.Suit);

	// Glow behind the card (Selected / NEW).
	Glow = PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::Magenta), FVector2D(-110.f, -110.f), FullSize + FVector2D(220.f), 0.f);

	// Frame: 4 px, chamfer 24. Specials: magenta/black tape that shifts with the tilt.
	if (Card.bSpecial)
	{
		UCanvasPanel* TapeClip = CoreKitBasicUIOverlay::AddChamferClip(Tree, Root, FVector2D::ZeroVector, FullSize, 24.f, TEXT("FrameTapeClip"));
		PlaceImage(Tree, TapeClip, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot), FVector2D::ZeroVector, FullSize);
		FrameTape = PlaceImage(Tree, TapeClip, UCoreKitBasicUIStyle::Tape(8, CoreKitBasicUIColor::Magenta), FVector2D(-48.f, -48.f), FullSize + FVector2D(96.f));
	}
	else
	{
		FrameSolid = PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Chamfer(24, FrameColor()), FVector2D::ZeroVector, FullSize);
	}

	// Body (Hull, chamfer 22) and scene (352 x 250 at the top, bleeding).
	PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Chamfer(22, CoreKitBasicUIColor::Hull), FVector2D(4.f, 4.f), FVector2D(352.f, 532.f));
	UCanvasPanel* Scene = CoreKitBasicUIOverlay::AddChamferClip(Tree, Root, FVector2D(4.f, 4.f), FullScene, 22.f, TEXT("SceneClip"));
	BuildArt(Scene, FullScene, false);

	// Top row: suit + domain, key hint.
	const FText SuitLabel = Card.SuitLabel.IsEmpty() ? DefaultSuitLabel(Card.Suit) : Card.SuitLabel;
	CoreKitBasicUIBuild::FTextStyle SuitStyle = CoreKitBasicUIBuild::WithShadow(CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 700, AccentColor, 0.24f));
	UTextBlock* Suit = CoreKitBasicUIBuild::Text(Tree, SuitLabel.ToUpper(), SuitStyle, TEXT("SuitLabel"));
	CoreKitBasicUIBuild::Place(Root, Suit, FAnchors(0.f, 0.f), FVector2D(26.f, 39.f), FVector2D::ZeroVector, FVector2D(0.f, 0.5f), 5);
	Keycap = Tree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass(), TEXT("KeyHint"));
	Keycap->SetVisibility(ESlateVisibility::Collapsed);
	CoreKitBasicUIBuild::Place(Root, Keycap, FAnchors(0.f, 0.f), FVector2D(334.f, 22.f), FVector2D::ZeroVector, FVector2D(1.f, 0.f), 5);

	// Badge 58 x 58 at (22, 214) of the body: Soot, chamfer 11, 2 px inner outline in the accent, suit glyph 32 px.
	const FVector2D BadgePos(26.f, 218.f);
	PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Chamfer(11, CoreKitBasicUIColor::Soot), BadgePos, FVector2D(58.f), 1.f, 6);
	CoreKitBasicUIOverlay::AddChamferOutline(Tree, Root, BadgePos, FVector2D(58.f), 11.f, 2.f, AccentColor, 6);
	PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Image(UCoreKitBasicUIStyle::SuitIcon(Card.Suit), SuitColor, FVector2D(32.f)), BadgePos + FVector2D(13.f), FVector2D(32.f), 1.f, 6);

	// Text block from y 286 of the body, margins 26, bottom 22. The footer is pinned to the bottom; chip + name +
	// effect live in a fixed area above it (FitText shrinks one step, then ellipsizes, when a text is too long).
	constexpr float TextTop = 290.f;
	constexpr float TextBottom = 518.f;
	UVerticalBox* FooterBox = CoreKitBasicUIBuild::Column(Tree, TEXT("FooterBox"));
	CoreKitBasicUIBuild::AddV(FooterBox, CoreKitBasicUIBuild::Box(Tree, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Seam)), 0.f, 1.f), FMargin(0.f));
	FooterText = CoreKitBasicUIBuild::Text(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Steel, 0.12f), TEXT("Footer"));
	CoreKitBasicUIBuild::AddV(FooterBox, FooterText, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	USizeBox* FooterSize = CoreKitBasicUIBuild::Box(Tree, FooterBox, 300.f, 0.f);
	FooterSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CoreKitBasicUIBuild::Place(Root, FooterSize, FAnchors(0.f, 0.f), FVector2D(30.f, TextBottom), FVector2D::ZeroVector, FVector2D(0.f, 1.f), 5);
	SetFooter(FooterOverride);
	float FooterHeight = 30.f;
	if (FSlateApplication::IsInitialized())
	{
		const TSharedRef<SWidget> FooterSlate = FooterSize->TakeWidget();
		FooterSlate->SlatePrepass(1.f);
		FooterHeight = FMath::Max(FooterHeight, static_cast<float>(FooterSlate->GetDesiredSize().Y));
	}
	const float TextMax = FMath::Max(40.f, TextBottom - FooterHeight - 8.f - TextTop);

	UVerticalBox* Column = CoreKitBasicUIBuild::Column(Tree, TEXT("TextColumn"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Column->SetClipping(EWidgetClipping::ClipToBounds);
	CoreKitBasicUIBuild::Place(Root, Column, FAnchors(0.f, 0.f), FVector2D(30.f, TextTop), FVector2D(300.f, TextMax), FVector2D::ZeroVector, 5);
	TextColumn = Column;
	const FText ChipText = Card.bSpecial ? COREKIT_BASICUI_TEXT("ChipSpecial", "SPECIAL · UNIQUE") : COREKIT_BASICUI_TEXT("ChipStat", "STAT · STACKS");
	UBorder* Chip = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(Card.bSpecial ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::Plate), FMargin(10.f, 5.f), TEXT("TypeChip"));
	Chip->SetContent(CoreKitBasicUIBuild::Text(Tree, ChipText, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 700, Card.bSpecial ? CoreKitBasicUIColor::Soot : CoreKitBasicUIColor::TextDim, 0.16f)));
	CoreKitBasicUIBuild::AddV(Column, Chip, FMargin(0.f), HAlign_Left);
	UTextBlock* Name = CoreKitBasicUIBuild::Text(Tree, Card.Name.ToUpper(), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(40.f), 700), TEXT("CardName"));
	Name->SetWrapTextAt(300.f);
	Name->SetLineHeightPercentage(0.82f);
	CoreKitBasicUIBuild::AddV(Column, Name, FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	NameText = Name;
	UWrapBox* Effect = Tree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("Effect"));
	Effect->SetExplicitWrapSize(true);
	Effect->SetWrapSize(300.f);
	Effect->SetInnerSlotPadding(FVector2D(0.f, 2.f));
	EffectBox = Effect;
	BuildEffectText(Effect, AccentColor);
	CoreKitBasicUIBuild::AddV(Column, Effect, FMargin(0.f, 8.f, 0.f, 0.f), HAlign_Left);
	FitText(TextMax);

	// Shine: white radial 0.16 that follows the cursor, only with tilt.
	UCanvasPanel* ShineClip = CoreKitBasicUIOverlay::AddChamferClip(Tree, Root, FVector2D(4.f, 4.f), FVector2D(352.f, 532.f), 22.f, TEXT("ShineClip"), 7);
	Shine = PlaceImage(Tree, ShineClip, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WhiteA(0.16f)), FVector2D(-210.f, -210.f), FVector2D(420.f), 0.f);

	// Focus outline: 3 px magenta, 8 px outside the card.
	for (UImage* Edge : CoreKitBasicUIOverlay::AddChamferOutline(Tree, Root, FVector2D(-11.f, -11.f), FullSize + FVector2D(22.f), 0.f, 3.f, CoreKitBasicUIColor::Magenta, 8))
	{
		Edge->SetVisibility(bFocusOutline ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		FocusEdges.Add(Edge);
	}

	UBorder* Tag = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(AccentColor), FMargin(6.f, 2.f), TEXT("NewTag"));
	Tag->SetContent(CoreKitBasicUIBuild::Text(Tree, COREKIT_BASICUI_TEXT("New", "NEW"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 700, CoreKitBasicUIColor::Soot, 0.14f)));
	CoreKitBasicUIBuild::Place(Root, Tag, FAnchors(0.f, 0.f), FVector2D(FullSize.X * 0.5f, -6.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f), 9);
	NewTag = Tag;
}

void UCoreKitBasicUICardWidget::BuildSmall()
{
	UWidgetTree* Tree = WidgetTree;
	const FVector2D Size = VariantSize(Variant, ThumbSize);
	const FLinearColor AccentColor = Accent();
	const bool bMini = Variant == ECoreKitBasicUICardVariant::Mini;
	const bool bThumb = Variant == ECoreKitBasicUICardVariant::Thumb;

	float FrameCut = 8.f;
	float Inset = 0.f;
	float Band = 3.f;
	float LabelPx = 11.f;
	float LabelBottom = 3.f;
	switch (Variant)
	{
	case ECoreKitBasicUICardVariant::Mini: FrameCut = 8.f; Inset = 2.f; Band = 3.f; LabelPx = 11.f; LabelBottom = 3.f; break;
	case ECoreKitBasicUICardVariant::Victory: FrameCut = 10.f; Inset = Card.bSpecial ? 3.f : 0.f; Band = 4.f; LabelPx = 13.f; LabelBottom = 7.f; break;
	case ECoreKitBasicUICardVariant::Defeat: FrameCut = 8.f; Inset = Card.bSpecial ? 3.f : 0.f; Band = 3.f; LabelPx = 12.f; LabelBottom = 6.f; break;
	default: FrameCut = FMath::Max(2.f, FMath::RoundToFloat(Size.X * 0.2f)); Inset = 0.f; Band = 0.f; break;
	}

	Glow = PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), AccentColor), -Size * 0.6f, Size * 2.2f, 0.f);

	// Frame: specials always get the 8 px tape; the mini card has a 2 px Plate frame.
	if (Card.bSpecial && !bThumb)
	{
		UCanvasPanel* TapeClip = CoreKitBasicUIOverlay::AddChamferClip(Tree, Root, FVector2D::ZeroVector, Size, FrameCut, TEXT("FrameTapeClip"));
		PlaceImage(Tree, TapeClip, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot), FVector2D::ZeroVector, Size);
		FrameTape = PlaceImage(Tree, TapeClip, UCoreKitBasicUIStyle::Tape(8, AccentColor), FVector2D(-12.f), Size + FVector2D(24.f));
		Inset = FMath::Max(Inset, 2.f);
	}
	else if (bMini)
	{
		FrameSolid = PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Chamfer(FMath::RoundToInt(FrameCut), CoreKitBasicUIColor::Plate), FVector2D::ZeroVector, Size);
	}

	const FVector2D BodySize = Size - FVector2D(Inset * 2.f);
	const float BodyCut = FMath::Max(1.f, FrameCut - (Inset > 0.f ? 1.f : 0.f));
	UCanvasPanel* Body = CoreKitBasicUIOverlay::AddChamferClip(Tree, Root, FVector2D(Inset), BodySize, BodyCut, TEXT("ArtClip"));
	BuildArt(Body, BodySize, true);

	// Accent band: across the top (the mini card draws it over its frame, the end-screen cards inside the body).
	if (Band > 0.f)
	{
		if (bMini)
		{
			PlaceImage(Tree, Root, UCoreKitBasicUIStyle::Solid(AccentColor), FVector2D(2.f, 2.f), FVector2D(Size.X - 4.f, Band), 1.f, 3);
		}
		else if (!Card.bSpecial)
		{
			PlaceImage(Tree, Body, UCoreKitBasicUIStyle::Solid(AccentColor), FVector2D::ZeroVector, FVector2D(BodySize.X, Band), 1.f, 3);
		}
	}

	if (!bThumb)
	{
		FString Label;
		FLinearColor LabelColor = CoreKitBasicUIColor::White;
		const FString Count = FString::Printf(TEXT("×%d"), FMath::Max(1, Card.Copies));
		if (Variant == ECoreKitBasicUICardVariant::Victory)
		{
			const FString Abbrev = Card.Abbrev.ToString();
			Label = Card.bSpecial ? Abbrev : (Abbrev.IsEmpty() ? Count : Abbrev + TEXT(" ") + Count);
			LabelColor = Card.bSpecial ? AccentColor : CoreKitBasicUIColor::White;
		}
		else
		{
			Label = Card.bSpecial ? FString(TEXT("★")) : Count;
			LabelColor = Card.bSpecial ? AccentColor : CoreKitBasicUIColor::White;
		}
		UTextBlock* Text = CoreKitBasicUIBuild::Text(Tree, FText::FromString(Label), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(LabelPx), 700, LabelColor, 0.f), TEXT("CountLabel"));
		Text->SetJustification(ETextJustify::Center);
		CoreKitBasicUIBuild::Place(Root, Text, FAnchors(0.f, 0.f), FVector2D(Size.X * 0.5f, Size.Y - LabelBottom), FVector2D::ZeroVector, FVector2D(0.5f, 1.f), 4);
	}

	UBorder* Tag = CoreKitBasicUIBuild::Panel(Tree, UCoreKitBasicUIStyle::Solid(AccentColor), FMargin(4.f, 1.f), TEXT("NewTag"));
	Tag->SetContent(CoreKitBasicUIBuild::Text(Tree, COREKIT_BASICUI_TEXT("NewSmall", "NEW"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(9.f), 700, CoreKitBasicUIColor::Soot, 0.14f)));
	CoreKitBasicUIBuild::Place(Root, Tag, FAnchors(0.f, 0.f), FVector2D(Size.X * 0.5f, -4.f), FVector2D::ZeroVector, FVector2D(0.5f, 1.f), 9);
	NewTag = Tag;
}

void UCoreKitBasicUICardWidget::ApplyKeyHint()
{
	if (!Keycap)
	{
		return;
	}
	const bool bVisible = Variant == ECoreKitBasicUICardVariant::Full && (!KeyHint.IsEmpty() || KeyHintKey.IsValid());
	Keycap->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bVisible)
	{
		return;
	}
	if (KeyHintKey.IsValid())
	{
		Keycap->SetFixedText(FText::GetEmpty());
		Keycap->SetKey(KeyHintKey);
	}
	else
	{
		Keycap->SetFixedText(KeyHint);
	}
}

void UCoreKitBasicUICardWidget::ApplyStateLook()
{
	const bool bFull = Variant == ECoreKitBasicUICardVariant::Full;
	float LiftTo = 0.f;
	float ScaleTo = 1.f;
	float BrightTo = bFull ? IdleBrightness : 1.f;
	ECoreKitBasicUIMotion Curve = ECoreKitBasicUIMotion::Settle;
	float Duration = 0.12f;
	switch (State)
	{
	case ECoreKitBasicUICardState::Hover:
		LiftTo = bFull ? -10.f : 0.f;
		ScaleTo = bFull ? 1.02f : 1.f;
		BrightTo = 1.f;
		break;
	case ECoreKitBasicUICardState::Selected:
		LiftTo = bFull ? -24.f : 0.f;
		ScaleTo = bFull ? 1.08f : 1.f;
		BrightTo = 1.f;
		Curve = ECoreKitBasicUIMotion::Slam;
		Duration = 0.25f;
		break;
	case ECoreKitBasicUICardState::Dimmed:
		BrightTo = 0.6f;
		Duration = 0.16f;
		break;
	default:
		break;
	}
	Lift.Start(LiftTo, Curve, Duration);
	Scale.Start(ScaleTo, Curve, Duration);
	Brightness.Start(BrightTo, ECoreKitBasicUIMotion::Settle, 0.16f);
	if (State == ECoreKitBasicUICardState::Selected)
	{
		GlowAlpha.Start(0.55f, ECoreKitBasicUIMotion::Settle, 0.25f);
	}
	else if (!bNew)
	{
		GlowAlpha.Start(0.f, ECoreKitBasicUIMotion::Exit, 0.14f);
	}
	if (FrameSolid && bFull)
	{
		FrameSolid->SetColorAndOpacity(State == ECoreKitBasicUICardState::Selected ? CoreKitBasicUIColor::Magenta : FrameColor());
	}
}

void UCoreKitBasicUICardWidget::ApplyTilt()
{
	if (!Root)
	{
		return;
	}
	const bool bFull = Variant == ECoreKitBasicUICardVariant::Full;
	const FVector2D T = bFull && !CoreKitBasicUIMotion::IsReducedMotion() ? SmoothTilt : FVector2D::ZeroVector;
	const float S = Scale.Value > 0.f ? Scale.Value : 1.f;

	// Perspective approximated in 2D: rotateY(+-12) narrows and skews vertically, rotateX(+-9) shortens and skews
	// horizontally (Slate render transforms are affine).
	FWidgetTransform RootTransform;
	RootTransform.Translation = FVector2D(0.f, Lift.Value);
	RootTransform.Scale = FVector2D(S * (1.f - 0.022f * FMath::Abs(T.X)), S * (1.f - 0.012f * FMath::Abs(T.Y)));
	RootTransform.Shear = FVector2D(T.Y * 2.f, -T.X * 2.5f);
	Root->SetRenderTransform(RootTransform);

	if (!bFull)
	{
		return;
	}
	CoreKitBasicUIBuild::SetOffset(SceneGroup, FVector2D(-T.X * 10.f, -T.Y * 7.f));
	CoreKitBasicUIBuild::SetOffset(EffectGroup, FVector2D(T.X * 7.f, T.Y * 5.f));
	CoreKitBasicUIBuild::SetOffset(NameText, FVector2D(T.X * 4.f, T.Y * 3.f));
	CoreKitBasicUIBuild::SetOffset(FrameTape, FVector2D(T.X * 24.f, T.Y * 24.f));
	if (Shine)
	{
		const float Amount = FMath::Clamp(T.Size() * 3.f, 0.f, 1.f);
		Shine->SetRenderOpacity(Amount);
		CoreKitBasicUIBuild::SetOffset(Shine, FVector2D((T.X + 1.f) * 0.5f * 352.f, (T.Y + 1.f) * 0.5f * 532.f));
	}
}

void UCoreKitBasicUICardWidget::TickMotion(float DeltaSeconds)
{
	Lift.Tick(DeltaSeconds);
	Scale.Tick(DeltaSeconds);
	Brightness.Tick(DeltaSeconds);
	GlowAlpha.Tick(DeltaSeconds);
	NewPop.Tick(DeltaSeconds);

	// 120 ms ease-out toward the target tilt.
	const float Follow = 1.f - FMath::Exp(-DeltaSeconds / 0.04f);
	SmoothTilt = CoreKitBasicUIMotion::IsReducedMotion() ? FVector2D::ZeroVector : FMath::Lerp(SmoothTilt, Tilt, Follow);
	ApplyTilt();

	const float B = Brightness.Value > 0.f ? Brightness.Value : 1.f;
	SetColorAndOpacity(FLinearColor(B, B, B, 1.f));
	if (Glow)
	{
		Glow->SetRenderOpacity(FMath::Clamp(GlowAlpha.Value, 0.f, 1.f));
	}
	if (NewTag && bNew)
	{
		CoreKitBasicUIBuild::SetTransform(NewTag, FVector2D::ZeroVector, FMath::Max(0.f, NewPop.Value), 0.f);
	}
}

#undef LOCTEXT_NAMESPACE
