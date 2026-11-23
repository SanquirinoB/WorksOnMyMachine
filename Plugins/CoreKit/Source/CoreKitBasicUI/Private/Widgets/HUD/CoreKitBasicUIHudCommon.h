#pragma once

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"

/** Private building helpers shared by the HUD widget files. Sizes are CSS px of the 1920 x 1080 mockups. */
namespace CoreKitBasicUIHudBuild
{
	using CoreKitBasicUIBuild::FTextStyle;

	/** HUD text style with the .sh legibility shadow. */
	inline FTextStyle Sh(FTextStyle Style)
	{
		Style.bShadow = true;
		return Style;
	}

	/** Display (Saira Condensed) at a CSS px size. */
	inline FTextStyle D(float CssPx, int32 Weight = 700, FLinearColor Color = CoreKitBasicUIColor::White, float Tracking = 0.f)
	{
		FTextStyle Style = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(CssPx), Weight, Color);
		Style.Tracking = Tracking;
		return Style;
	}

	/** JetBrains Mono at a CSS px size. */
	inline FTextStyle M(float CssPx, int32 Weight = 500, FLinearColor Color = CoreKitBasicUIColor::KeyLight, float Tracking = 0.18f)
	{
		return CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(CssPx), Weight, Color, Tracking);
	}

	inline UImage* Solid(UWidgetTree* Tree, const FLinearColor& Color, FName Name = NAME_None)
	{
		return CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(Color), Name);
	}

	inline USpacer* Gap(UWidgetTree* Tree, float Width, float Height = 1.f)
	{
		USpacer* Spacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Spacer->SetSize(FVector2D(Width, Height));
		return Spacer;
	}

	inline UCoreKitBasicUITabularNumber* Tabular(UWidgetTree* Tree, const FTextStyle& Style, FName Name = NAME_None)
	{
		UCoreKitBasicUITabularNumber* Number = Tree->ConstructWidget<UCoreKitBasicUITabularNumber>(UCoreKitBasicUITabularNumber::StaticClass(), Name);
		Number->Setup(Style);
		return Number;
	}

	/** Horizontal scale for bar fills (pivot at the left, or at the right with bFromRight). Hidden at 0. */
	inline void SetFillX(UWidget* Widget, float Fraction, bool bFromRight = false)
	{
		if (!Widget)
		{
			return;
		}
		const float F = FMath::Clamp(Fraction, 0.f, 1.f);
		Widget->SetRenderTransformPivot(FVector2D(bFromRight ? 1.f : 0.f, 0.5f));
		FWidgetTransform Transform = Widget->GetRenderTransform();
		Transform.Scale = FVector2D(FMath::Max(F, 0.001f), 1.f);
		Widget->SetRenderTransform(Transform);
		Widget->SetVisibility(F <= 0.001f ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}

	/** Translation + uniform scale + angle around the current pivot. */
	inline void SetXform(UWidget* Widget, FVector2D Offset, float Scale = 1.f, float Angle = 0.f)
	{
		if (!Widget)
		{
			return;
		}
		FWidgetTransform Transform;
		Transform.Translation = Offset;
		Transform.Scale = FVector2D(Scale, Scale);
		Transform.Angle = Angle;
		Widget->SetRenderTransform(Transform);
	}

	inline void Show(UWidget* Widget, bool bShow)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	/** Fraction of a keyframe segment [From, To] of a 0..1 timeline. */
	inline float Seg(float Alpha, float From, float To)
	{
		return To > From ? FMath::Clamp((Alpha - From) / (To - From), 0.f, 1.f) : (Alpha >= To ? 1.f : 0.f);
	}

	inline float Ease(ECoreKitBasicUIMotion Token, float Alpha)
	{
		return CoreKitBasicUIMotion::Ease(Token, Alpha);
	}

	/** Duration of a motion token (editable in Project Settings > CoreKitBasicUI Motion). */
	inline float Token(ECoreKitBasicUIMotion Motion)
	{
		return GetDefault<UCoreKitBasicUIMotionSettings>()->Duration(Motion);
	}

	/** SetText only when the string changes (avoids a layout invalidation every frame). */
	inline void SetTextIfChanged(UTextBlock* Text, const FText& Value)
	{
		if (Text && !Text->GetText().EqualTo(Value))
		{
			Text->SetText(Value);
		}
	}

	inline void SetTextColor(UTextBlock* Text, const FLinearColor& Color)
	{
		if (Text)
		{
			Text->SetColorAndOpacity(FSlateColor(Color));
		}
	}

	/** Soft radial dark vignette behind a HUD block (rgba(10,10,12,.45-.6)). */
	inline UImage* Vignette(UWidgetTree* Tree, float Alpha)
	{
		return CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_RadialFalloff"), CoreKitBasicUIColor::SootA(Alpha)));
	}

	/** Hazard tape strip: Soot plate behind tiled stripes. */
	inline UOverlay* TapeStrip(UWidgetTree* Tree, int32 Stripe, const FLinearColor& Color, float Width, float Height, FName Name = NAME_None)
	{
		UOverlay* Tape = CoreKitBasicUIBuild::Overlay(Tree, Name);
		CoreKitBasicUIBuild::AddO(Tape, Solid(Tree, CoreKitBasicUIColor::Soot));
		CoreKitBasicUIBuild::AddO(Tape, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Tape(Stripe, Color)));
		USizeBox* Box = CoreKitBasicUIBuild::Box(Tree, nullptr, Width, Height);
		CoreKitBasicUIBuild::AddO(Tape, Box);
		Tape->SetClipping(EWidgetClipping::ClipToBounds);
		return Tape;
	}
}
