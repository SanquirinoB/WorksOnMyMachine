#pragma once

#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"

class UBorder;
class UButton;
class USizeBox;
class UTextBlock;
class UWidget;
class UWidgetTree;

namespace DelerictUI
{
using Family = ECoreKitBasicUIFontFamily;
using CoreKitBasicUIColor::Hex;
inline const FLinearColor Ink = Hex(0xEDF3F4);
inline const FLinearColor Muted = Hex(0xADBDC2);
inline const FLinearColor Accent = Hex(0xF19A55);
inline const FLinearColor Panel = Hex(0x0D181E, .97f);
inline const FLinearColor Edge = Hex(0x40515A);
inline const FLinearColor Field = Hex(0x16252E);
inline const FLinearColor Divider = Hex(0x2B3B43);

UTextBlock* Label(UWidgetTree& Tree, const FText& Text, float Size, Family Face = Family::Body,
    int32 Weight = 500, FLinearColor Color = Ink, float Tracking = 0.f);
UBorder* Surface(UWidgetTree& Tree, FLinearColor Color, FMargin Padding = FMargin(0), float Outline = 0.f);
USizeBox* Box(UWidgetTree& Tree, UWidget* Child, float Width, float Height);
FButtonStyle ButtonStyle(FLinearColor Normal = Field, FLinearColor Stroke = Edge, float Outline = 1.f);
FButtonStyle AccentButtonStyle();
UButton* Button(UWidgetTree& Tree, UObject& Owner, FName Name, FName Callback, UWidget* Content);
}
