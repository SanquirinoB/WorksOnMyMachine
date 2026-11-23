#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

// ---------------------------------------------------------------------------------------------------------------
// Tabular number

void UCoreKitBasicUITabularNumber::Setup(const CoreKitBasicUIBuild::FTextStyle& InStyle)
{
	Style = InStyle;
	DigitWidth = 0.f;
	Rebuild();
}

void UCoreKitBasicUITabularNumber::BuildTree()
{
	Cells = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Cells"));
	WidgetTree->RootWidget = Cells;
	Rebuild();
}

void UCoreKitBasicUITabularNumber::SetValue(const FString& Value)
{
	if (Value == Current && Glyphs.Num() == Value.Len())
	{
		return;
	}
	Current = Value;
	Rebuild();
}

void UCoreKitBasicUITabularNumber::SetColor(const FLinearColor& Color)
{
	Style.Color = Color;
	for (UTextBlock* Glyph : Glyphs)
	{
		Glyph->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UCoreKitBasicUITabularNumber::Rebuild()
{
	if (!Cells || !WidgetTree)
	{
		return;
	}
	const FSlateFontInfo Font = UCoreKitBasicUIStyle::Font(Style.Family, Style.Weight, Style.Size, Style.Tracking);
	if (DigitWidth <= 0.f)
	{
		float Widest = Style.Size * 0.62f;
		if (FSlateApplication::IsInitialized())
		{
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			Widest = 0.f;
			for (TCHAR Digit = TEXT('0'); Digit <= TEXT('9'); ++Digit)
			{
				Widest = FMath::Max(Widest, static_cast<float>(Measure->Measure(FString(1, &Digit), Font).X));
			}
		}
		DigitWidth = FMath::CeilToFloat(Widest);
	}
	Cells->ClearChildren();
	Glyphs.Reset();
	for (TCHAR Char : Current)
	{
		UTextBlock* Glyph = CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(FString(1, &Char)), Style);
		Glyph->SetJustification(ETextJustify::Center);
		const bool bDigit = FChar::IsDigit(Char);
		USizeBox* Cell = CoreKitBasicUIBuild::Box(WidgetTree, Glyph, bDigit ? DigitWidth : 0.f, 0.f);
		CoreKitBasicUIBuild::AddH(Cells, Cell, FMargin(0.f), VAlign_Bottom, HAlign_Center);
		Glyphs.Add(Glyph);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Action button

void UCoreKitBasicUIActionButton::Setup(const FText& InLabel, const FText& InSublabel, ECoreKitBasicUIButtonVariant InVariant, float InHeight, float InLabelPx, bool bInTriangle)
{
	LabelText = InLabel;
	SublabelText = InSublabel;
	Variant = InVariant;
	Height = InHeight;
	LabelPx = InLabelPx;
	bTriangle = bInTriangle;
	if (Label)
	{
		Label->SetText(LabelText);
		Sublabel->SetText(SublabelText);
		Sublabel->SetVisibility(SublabelText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		Triangle->SetVisibility(bTriangle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Restyle();
	}
}

void UCoreKitBasicUIActionButton::BuildTree()
{
	Button = WidgetTree->ConstructWidget<UCoreKitButton>(UCoreKitButton::StaticClass(), TEXT("Button"));
	Button->SetBaseStyle(UCoreKitBasicUIStyle::ClearButtonStyle());
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Button, 0.f, Height, TEXT("Size"));

	UOverlay* Stack = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("Stack"));
	if (UButtonSlot* StackSlot = Cast<UButtonSlot>(Button->AddChild(Stack)))
	{
		// The plate fills the whole button (UButtonSlot centres its content by default).
		StackSlot->SetPadding(FMargin(0.f));
		StackSlot->SetHorizontalAlignment(HAlign_Fill);
		StackSlot->SetVerticalAlignment(VAlign_Fill);
	}
	// The magenta inset is a full chamfered plate under the face; the face starts InsetWidth px in with a chamfer
	// smaller by the same amount, so the 45-degree corner cuts the inset exactly like the mockup's clip-path.
	Inset = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Chamfer(18, CoreKitBasicUIColor::Magenta), TEXT("Inset"));
	CoreKitBasicUIBuild::AddO(Stack, Inset);
	Plate = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Chamfer(18, CoreKitBasicUIColor::Hull), TEXT("Plate"));
	CoreKitBasicUIBuild::AddO(Stack, Plate);

	UHorizontalBox* Content = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Content"));
	CoreKitBasicUIBuild::AddO(Stack, Content, HAlign_Fill, VAlign_Center, FMargin(34.f, 0.f, 34.f, 0.f));
	Triangle = CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Triangle"), CoreKitBasicUIColor::Soot, 20.f, TEXT("Triangle"));
	CoreKitBasicUIBuild::AddH(Content, Triangle, FMargin(0.f, 0.f, 16.f, 0.f));
	Label = CoreKitBasicUIBuild::Text(WidgetTree, LabelText, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(LabelPx), 700), TEXT("Label"));
	CoreKitBasicUIBuild::AddH(Content, Label, FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
	Keycap = WidgetTree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass(), TEXT("Keycap"));
	Keycap->SetVisibility(ESlateVisibility::Collapsed);
	CoreKitBasicUIBuild::AddH(Content, Keycap, FMargin(12.f, 0.f, 0.f, 0.f));
	Sublabel = CoreKitBasicUIBuild::Text(WidgetTree, SublabelText, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.14f), TEXT("Sublabel"));
	CoreKitBasicUIBuild::AddH(Content, Sublabel, FMargin(12.f, 0.f, 0.f, 0.f));
	Setup(LabelText, SublabelText, Variant, Height, LabelPx, bTriangle);
}

void UCoreKitBasicUIActionButton::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button)
	{
		Button->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleActivated);
		Button->OnSelected.AddUniqueDynamic(this, &ThisClass::HandleSelected);
		Button->OnDeselected.AddUniqueDynamic(this, &ThisClass::HandleDeselected);
	}
	Restyle();
}

void UCoreKitBasicUIActionButton::SetKeyHint(UInputAction* Action, FKey Key)
{
	if (!Keycap)
	{
		return;
	}
	Keycap->SetAction(Action);
	Keycap->SetKey(Key);
	Keycap->SetVisibility(ESlateVisibility::HitTestInvisible);
	Sublabel->SetVisibility(ESlateVisibility::Collapsed);
}

void UCoreKitBasicUIActionButton::Select()
{
	if (Button)
	{
		Button->Select();
	}
}

bool UCoreKitBasicUIActionButton::IsSelected() const
{
	return Button && Button->IsSelected();
}

void UCoreKitBasicUIActionButton::HandleActivated(UCoreKitButton*)
{
	OnActivated.Broadcast(this);
}

void UCoreKitBasicUIActionButton::HandleSelected(UCoreKitButton*)
{
	bSelectedVisual = true;
	Restyle();
	OnSelected.Broadcast(this);
}

void UCoreKitBasicUIActionButton::HandleDeselected(UCoreKitButton*)
{
	bSelectedVisual = false;
	Restyle();
}

void UCoreKitBasicUIActionButton::Restyle()
{
	if (!Plate)
	{
		return;
	}
	FLinearColor Fill = CoreKitBasicUIColor::Hull;
	FLinearColor Text = CoreKitBasicUIColor::White;
	FLinearColor Sub = CoreKitBasicUIColor::Muted;
	float InsetWidth = 0.f;
	const bool bSel = bSelectedVisual;
	switch (Variant)
	{
	case ECoreKitBasicUIButtonVariant::MenuList:
		Fill = bSel ? CoreKitBasicUIColor::Plate : CoreKitBasicUIColor::Hull;
		Text = bSel ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White;
		break;
	case ECoreKitBasicUIButtonVariant::Action:
		Fill = bSel ? CoreKitBasicUIColor::White : CoreKitBasicUIColor::Hull;
		Text = bSel ? CoreKitBasicUIColor::Soot : CoreKitBasicUIColor::White;
		Sub = bSel ? CoreKitBasicUIColor::Shade : CoreKitBasicUIColor::Muted;
		InsetWidth = bSel ? 8.f : 0.f;
		break;
	case ECoreKitBasicUIButtonVariant::Primary:
		Fill = CoreKitBasicUIColor::White;
		Text = CoreKitBasicUIColor::Soot;
		Sub = CoreKitBasicUIColor::Shade;
		InsetWidth = bSel ? 10.f : 8.f;
		break;
	case ECoreKitBasicUIButtonVariant::Confirm:
		Fill = bSel ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.85f);
		Text = CoreKitBasicUIColor::Soot;
		Sub = CoreKitBasicUIColor::Soot;
		break;
	case ECoreKitBasicUIButtonVariant::Danger:
		Fill = bSel ? CoreKitBasicUIColor::Red : CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Red, 0.85f);
		Text = CoreKitBasicUIColor::Soot;
		Sub = CoreKitBasicUIColor::Soot;
		break;
	case ECoreKitBasicUIButtonVariant::Neutral:
		Fill = bSel ? CoreKitBasicUIColor::White : CoreKitBasicUIColor::Plate;
		Text = bSel ? CoreKitBasicUIColor::Soot : CoreKitBasicUIColor::White;
		break;
	}
	const int32 Cut = Height >= 70.f ? 20 : 18;
	Inset->SetBrush(UCoreKitBasicUIStyle::Chamfer(Cut, CoreKitBasicUIColor::Magenta));
	Inset->SetVisibility(InsetWidth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Plate->SetBrush(UCoreKitBasicUIStyle::Chamfer(FMath::Max(6, Cut - FMath::RoundToInt(InsetWidth)), Fill));
	if (UOverlaySlot* FaceSlot = Cast<UOverlaySlot>(Plate->Slot))
	{
		FaceSlot->SetPadding(FMargin(InsetWidth, 0.f, 0.f, 0.f));
	}
	Label->SetColorAndOpacity(FSlateColor(Text));
	Sublabel->SetColorAndOpacity(FSlateColor(Sub));
	Triangle->SetColorAndOpacity(Text);
}

// ---------------------------------------------------------------------------------------------------------------
// Keycap

TSharedRef<SWidget> UCoreKitBasicUIKeycap::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Edge = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(Accent), TEXT("Edge"));
		CoreKitBasicUIBuild::AddO(Root, Edge);
		Face = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Hull), FMargin(8.f, 2.f), TEXT("Face"));
		CoreKitBasicUIBuild::AddO(Root, Face, HAlign_Fill, VAlign_Fill, FMargin(1.5f, 1.5f, 1.5f, 4.f));
		UOverlay* Inner = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("Inner"));
		Face->SetContent(Inner);
		Icon = CoreKitBasicUIBuild::Image(WidgetTree, FSlateBrush(), TEXT("Icon"));
		CoreKitBasicUIBuild::AddO(Root, Icon, HAlign_Center, VAlign_Center);
		Label = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(15.f), 700, CoreKitBasicUIColor::White, 0.f), TEXT("Label"));
		CoreKitBasicUIBuild::AddO(Inner, Label, HAlign_Center, VAlign_Center);
		ApplyLook();
	}
	return UUserWidget::RebuildWidget();
}

void UCoreKitBasicUIKeycap::SetAccent(const FLinearColor& Color)
{
	Accent = Color;
	ApplyLook();
}

void UCoreKitBasicUIKeycap::SetFixedText(const FText& InText)
{
	FixedText = InText;
	if (Label)
	{
		Label->SetText(FixedText);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	ApplyLook();
}

void UCoreKitBasicUIKeycap::ApplyLook()
{
	if (!Edge)
	{
		return;
	}
	const bool bHasIcon = Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed && FixedText.IsEmpty();
	Edge->SetVisibility(bHasIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	Face->SetVisibility(bHasIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	Edge->SetColorAndOpacity(bHudSize ? CoreKitBasicUIColor::TextSoft : Accent);
	Face->SetBrushColor(bHudSize ? CoreKitBasicUIColor::SootA(0.6f) : CoreKitBasicUIColor::Hull);
	Face->SetPadding(bHudSize ? FMargin(6.f, 1.f) : FMargin(10.f, 4.f));
	if (Label)
	{
		CoreKitBasicUIBuild::ApplyTextStyle(Label, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(bHudSize ? 10.f : 15.f), 700, CoreKitBasicUIColor::White, 0.f));
	}
	if (Icon)
	{
		const float IconSize = bHudSize ? 22.f : 40.f;
		FSlateBrush Brush = Icon->GetBrush();
		Brush.ImageSize = FVector2D(IconSize, IconSize);
		Icon->SetBrush(Brush);
	}
}

void UCoreKitBasicUIKeycap::NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey)
{
	if (!FixedText.IsEmpty() && Label)
	{
		Label->SetText(FixedText);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (Icon)
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	ApplyLook();
}

void UCoreKitBasicUIActionButton::NativeDestruct()
{
    if (Button) {
        Button->OnActivated.RemoveDynamic(this, &ThisClass::HandleActivated);
        Button->OnSelected.RemoveDynamic(this, &ThisClass::HandleSelected);
        Button->OnDeselected.RemoveDynamic(this, &ThisClass::HandleDeselected);
    }
    Super::NativeDestruct();
}
