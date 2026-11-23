#pragma once

#include "Components/CanvasPanelSlot.h"
#include "Layout/Margin.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"
#include "Types/SlateEnums.h"

class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class UOverlay;
class UPanelWidget;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
class UWidgetTree;

/**
 * Helpers to build native widget trees in C++ (UWidgetTree::ConstructWidget + slot setup) with the design system.
 * Widgets built here keep stable names so a Widget Blueprint subclass can replace any part with BindWidget.
 */
namespace CoreKitBasicUIBuild
{
	struct FTextStyle
	{
		ECoreKitBasicUIFontFamily Family = ECoreKitBasicUIFontFamily::Display;
		int32 Weight = 700;
		float Size = 20.f;
		float Tracking = 0.f;
		FLinearColor Color = CoreKitBasicUIColor::White;
		bool bShadow = false;
		bool bUpper = false;
		ETextJustify::Type Justify = ETextJustify::Left;
	};

	inline FTextStyle Display(float Size, int32 Weight = 700, FLinearColor Color = CoreKitBasicUIColor::White) { FTextStyle S; S.Family = ECoreKitBasicUIFontFamily::Display; S.Size = Size; S.Weight = Weight; S.Color = Color; return S; }
	inline FTextStyle Body(float Size, int32 Weight = 400, FLinearColor Color = CoreKitBasicUIColor::White) { FTextStyle S; S.Family = ECoreKitBasicUIFontFamily::Body; S.Size = Size; S.Weight = Weight; S.Color = Color; return S; }
	inline FTextStyle Mono(float Size, int32 Weight = 500, FLinearColor Color = CoreKitBasicUIColor::Steel, float Tracking = 0.18f) { FTextStyle S; S.Family = ECoreKitBasicUIFontFamily::Mono; S.Size = Size; S.Weight = Weight; S.Color = Color; S.Tracking = Tracking; return S; }
	inline FTextStyle Stencil(float Size, FLinearColor Color = CoreKitBasicUIColor::White) { FTextStyle S; S.Family = ECoreKitBasicUIFontFamily::Stencil; S.Size = Size; S.Weight = 400; S.Color = Color; return S; }
	inline FTextStyle WithShadow(FTextStyle S) { S.bShadow = true; return S; }

	/**
	 * Converts a CSS px size from the 1920x1080 mockups to a UMG font size (UMG font size is in points at 96 DPI:
	 * size_pt = px * 0.75).
	 */
	inline float Px(float CssPx) { return CssPx * 0.75f; }

	COREKITBASICUI_API void ApplyTextStyle(UTextBlock* Text, const FTextStyle& Style);
	COREKITBASICUI_API UTextBlock* Text(UWidgetTree* Tree, const FText& Value, const FTextStyle& Style, FName Name = NAME_None);
	COREKITBASICUI_API UImage* Image(UWidgetTree* Tree, const FSlateBrush& Brush, FName Name = NAME_None);
	COREKITBASICUI_API UImage* Icon(UWidgetTree* Tree, FName Texture, FLinearColor Color, float Size, FName Name = NAME_None);
	COREKITBASICUI_API UBorder* Panel(UWidgetTree* Tree, const FSlateBrush& Brush, FMargin Padding = FMargin(0.f), FName Name = NAME_None);
	COREKITBASICUI_API UCanvasPanel* Canvas(UWidgetTree* Tree, FName Name = NAME_None);
	COREKITBASICUI_API UOverlay* Overlay(UWidgetTree* Tree, FName Name = NAME_None);
	COREKITBASICUI_API UHorizontalBox* Row(UWidgetTree* Tree, FName Name = NAME_None);
	COREKITBASICUI_API UVerticalBox* Column(UWidgetTree* Tree, FName Name = NAME_None);
	COREKITBASICUI_API USizeBox* Box(UWidgetTree* Tree, UWidget* Child, float Width, float Height, FName Name = NAME_None);

	/** Places a widget on a canvas. Anchors/Alignment 0..1; Position and Size in px; Size zero = auto size. */
	COREKITBASICUI_API UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FAnchors Anchors, FVector2D Position, FVector2D Size = FVector2D::ZeroVector, FVector2D Alignment = FVector2D::ZeroVector, int32 ZOrder = 0);
	/** Stretches a widget over the whole canvas (with optional inset). */
	COREKITBASICUI_API UCanvasPanelSlot* Fill(UCanvasPanel* Canvas, UWidget* Widget, FMargin Inset = FMargin(0.f), int32 ZOrder = 0);

	/** Adds to a horizontal/vertical box. Fill > 0 uses a fill rule with that weight. */
	COREKITBASICUI_API void AddH(UHorizontalBox* Box, UWidget* Widget, FMargin Padding = FMargin(0.f), EVerticalAlignment V = VAlign_Center, EHorizontalAlignment H = HAlign_Fill, float Fill = 0.f);
	COREKITBASICUI_API void AddV(UVerticalBox* Box, UWidget* Widget, FMargin Padding = FMargin(0.f), EHorizontalAlignment H = HAlign_Fill, EVerticalAlignment V = VAlign_Fill, float Fill = 0.f);
	COREKITBASICUI_API void AddO(UOverlay* Box, UWidget* Widget, EHorizontalAlignment H = HAlign_Fill, EVerticalAlignment V = VAlign_Fill, FMargin Padding = FMargin(0.f));

	/** Render-transform helpers used by tweens. */
	COREKITBASICUI_API void SetOffset(UWidget* Widget, FVector2D Offset);
	COREKITBASICUI_API void SetTransform(UWidget* Widget, FVector2D Offset, float Scale = 1.f, float AngleDeg = 0.f);
}
