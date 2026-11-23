#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace CoreKitBasicUIBuild
{
	template <typename T>
	T* Make(UWidgetTree* Tree, FName Name)
	{
		return Tree->ConstructWidget<T>(T::StaticClass(), Name);
	}

	void ApplyTextStyle(UTextBlock* Text, const FTextStyle& Style)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(UCoreKitBasicUIStyle::Font(Style.Family, Style.Weight, Style.Size, Style.Tracking));
		Text->SetColorAndOpacity(FSlateColor(Style.Color));
		Text->SetJustification(Style.Justify);
		if (Style.bShadow)
		{
			Text->SetShadowOffset(FVector2D(0.f, 1.f));
			Text->SetShadowColorAndOpacity(CoreKitBasicUIColor::SootA(0.9f));
		}
	}

	UTextBlock* Text(UWidgetTree* Tree, const FText& Value, const FTextStyle& Style, FName Name)
	{
		UTextBlock* Text = Make<UTextBlock>(Tree, Name);
		Text->SetText(Style.bUpper ? Value.ToUpper() : Value);
		ApplyTextStyle(Text, Style);
		return Text;
	}

	UImage* Image(UWidgetTree* Tree, const FSlateBrush& Brush, FName Name)
	{
		UImage* Image = Make<UImage>(Tree, Name);
		Image->SetBrush(Brush);
		return Image;
	}

	UImage* Icon(UWidgetTree* Tree, FName Texture, FLinearColor Color, float Size, FName Name)
	{
		return Image(Tree, UCoreKitBasicUIStyle::Image(Texture, Color, FVector2D(Size, Size)), Name);
	}

	UBorder* Panel(UWidgetTree* Tree, const FSlateBrush& Brush, FMargin Padding, FName Name)
	{
		UBorder* Border = Make<UBorder>(Tree, Name);
		Border->SetBrush(Brush);
		Border->SetPadding(Padding);
		Border->SetHorizontalAlignment(HAlign_Fill);
		Border->SetVerticalAlignment(VAlign_Fill);
		return Border;
	}

	UCanvasPanel* Canvas(UWidgetTree* Tree, FName Name) { return Make<UCanvasPanel>(Tree, Name); }
	UOverlay* Overlay(UWidgetTree* Tree, FName Name) { return Make<UOverlay>(Tree, Name); }
	UHorizontalBox* Row(UWidgetTree* Tree, FName Name) { return Make<UHorizontalBox>(Tree, Name); }
	UVerticalBox* Column(UWidgetTree* Tree, FName Name) { return Make<UVerticalBox>(Tree, Name); }

	USizeBox* Box(UWidgetTree* Tree, UWidget* Child, float Width, float Height, FName Name)
	{
		USizeBox* Box = Make<USizeBox>(Tree, Name);
		if (Width > 0.f)
		{
			Box->SetWidthOverride(Width);
		}
		if (Height > 0.f)
		{
			Box->SetHeightOverride(Height);
		}
		if (Child)
		{
			Box->AddChild(Child);
		}
		return Box;
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FAnchors Anchors, FVector2D Position, FVector2D Size, FVector2D Alignment, int32 ZOrder)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(Size.IsNearlyZero());
		if (!Size.IsNearlyZero())
		{
			CanvasSlot->SetSize(Size);
		}
		CanvasSlot->SetZOrder(ZOrder);
		return CanvasSlot;
	}

	UCanvasPanelSlot* Fill(UCanvasPanel* Canvas, UWidget* Widget, FMargin Inset, int32 ZOrder)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(Inset);
		CanvasSlot->SetZOrder(ZOrder);
		return CanvasSlot;
	}

	void AddH(UHorizontalBox* Box, UWidget* Widget, FMargin Padding, EVerticalAlignment V, EHorizontalAlignment H, float Fill)
	{
		UHorizontalBoxSlot* BoxSlot = Box->AddChildToHorizontalBox(Widget);
		BoxSlot->SetPadding(Padding);
		BoxSlot->SetVerticalAlignment(V);
		BoxSlot->SetHorizontalAlignment(H);
		if (Fill > 0.f)
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Fill;
			BoxSlot->SetSize(Size);
		}
	}

	void AddV(UVerticalBox* Box, UWidget* Widget, FMargin Padding, EHorizontalAlignment H, EVerticalAlignment V, float Fill)
	{
		UVerticalBoxSlot* BoxSlot = Box->AddChildToVerticalBox(Widget);
		BoxSlot->SetPadding(Padding);
		BoxSlot->SetHorizontalAlignment(H);
		BoxSlot->SetVerticalAlignment(V);
		if (Fill > 0.f)
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Fill;
			BoxSlot->SetSize(Size);
		}
	}

	void AddO(UOverlay* Box, UWidget* Widget, EHorizontalAlignment H, EVerticalAlignment V, FMargin Padding)
	{
		UOverlaySlot* BoxSlot = Box->AddChildToOverlay(Widget);
		BoxSlot->SetHorizontalAlignment(H);
		BoxSlot->SetVerticalAlignment(V);
		BoxSlot->SetPadding(Padding);
	}

	void SetOffset(UWidget* Widget, FVector2D Offset)
	{
		if (Widget)
		{
			FWidgetTransform T = Widget->GetRenderTransform();
			T.Translation = Offset;
			Widget->SetRenderTransform(T);
		}
	}

	void SetTransform(UWidget* Widget, FVector2D Offset, float Scale, float AngleDeg)
	{
		if (Widget)
		{
			FWidgetTransform T;
			T.Translation = Offset;
			T.Scale = FVector2D(Scale, Scale);
			T.Angle = AngleDeg;
			Widget->SetRenderTransform(T);
		}
	}
}
