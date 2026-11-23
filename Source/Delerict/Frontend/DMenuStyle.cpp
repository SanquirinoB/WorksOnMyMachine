#include "DMenuStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

namespace DelerictUI
{
UTextBlock* Label(UWidgetTree& Tree, const FText& Text, float Size, Family Face,
    int32 Weight, FLinearColor Color, float Tracking)
{
    auto* Item = Tree.ConstructWidget<UTextBlock>();
    Item->SetText(Text);
    if (Face == Family::Body)
    {
        Item->SetFont(FCoreStyle::GetDefaultFontStyle(Weight >= 600 ? TEXT("Bold") : TEXT("Regular"), Size));
    }
    else
    {
        Item->SetFont(UCoreKitBasicUIStyle::Font(Face, Weight, Size, Tracking));
    }
    Item->SetColorAndOpacity(Color);
    Item->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Item;
}

UBorder* Surface(UWidgetTree& Tree, FLinearColor Color, FMargin Padding, float Outline)
{
    auto* Item = Tree.ConstructWidget<UBorder>();
    Item->SetBrush(FSlateRoundedBoxBrush(Color, 0.f, Edge, Outline));
    Item->SetPadding(Padding);
    return Item;
}

USizeBox* Box(UWidgetTree& Tree, UWidget* Child, float Width, float Height)
{
    auto* Item = Tree.ConstructWidget<USizeBox>();
    if (Width > 0.f) { Item->SetWidthOverride(Width); }
    if (Height > 0.f) { Item->SetHeightOverride(Height); }
    Item->SetContent(Child);
    return Item;
}

FButtonStyle ButtonStyle(FLinearColor Normal, FLinearColor Stroke, float Outline)
{
    FButtonStyle Style;
    Style.Normal = FSlateRoundedBoxBrush(Normal, 0.f, Stroke, Outline);
    Style.Hovered = FSlateRoundedBoxBrush(Hex(0x23343D), 0.f, Accent, 2.f);
    Style.Pressed = FSlateRoundedBoxBrush(Hex(0x392A22), 0.f, Accent, 2.f);
    Style.Disabled = FSlateRoundedBoxBrush(Normal.CopyWithNewOpacity(Normal.A * .48f), 0.f, Stroke.CopyWithNewOpacity(Stroke.A * .45f), Outline);
    Style.NormalPadding = FMargin(0);
    Style.PressedPadding = FMargin(0);
    return Style;
}

FButtonStyle AccentButtonStyle()
{
    auto Style = ButtonStyle(Accent, Accent);
    Style.Hovered = FSlateColorBrush(Hex(0xFFB675));
    Style.Pressed = FSlateColorBrush(Hex(0xD78B4F));
    return Style;
}

UButton* Button(UWidgetTree& Tree, UObject& Owner, FName Name, FName Callback, UWidget* Content)
{
    auto* Item = Tree.ConstructWidget<UButton>(UButton::StaticClass(), Name);
    Item->SetStyle(ButtonStyle());
    Item->SetContent(Content);
    auto* Slot = CastChecked<UButtonSlot>(Content->Slot);
    Slot->SetHorizontalAlignment(HAlign_Fill);
    Slot->SetVerticalAlignment(VAlign_Fill);
    FScriptDelegate Clicked;
    Clicked.BindUFunction(&Owner, Callback);
    Item->OnClicked.Add(Clicked);
    return Item;
}
}
