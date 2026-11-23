#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUISafeDraw.h"

#include "Blueprint/WidgetTree.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Widgets/HUD/CoreKitBasicUIHudDraw.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "InputAction.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudLogic.h"
#include "Widgets/SLeafWidget.h"

// ---------------------------------------------------------------------------------------------------------------
// Block base

void UCoreKitBasicUIHudBlock::TickHud(float RealDeltaSeconds)
{
	// Only once the native tree exists (BuildTree runs when the Slate widget is first built).
	if (WidgetTree && WidgetTree->RootWidget)
	{
		TickMotion(RealDeltaSeconds);
	}
}

bool UCoreKitBasicUIHudBlock::IsReducedMotion() const
{
	if (CoreKitBasicUIMotion::IsReducedMotion())
	{
		return true;
	}
	const UCoreKitBasicUIContextSubsystem* Context = GetUIContext();
	return Context && Context->GetPreferences() && Context->GetPreferences()->bReducedMotion;
}

void UCoreKitBasicUIHudBlock::PlayHudCue(FName Cue) const
{
	PlayCue(Cue);
}

// ---------------------------------------------------------------------------------------------------------------
// Shape

class SCoreKitBasicUIHudShape : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCoreKitBasicUIHudShape) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	TArray<FVector2D> Points;
	FLinearColor Color = FLinearColor::White;

	FVector2D ComputeDesiredSize(float) const override { return FVector2D(16.f, 16.f); }

	int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer,
		const FWidgetStyle& Style, bool) const override
	{
		const FVector2f Size = FVector2f(Geometry.GetLocalSize());
		TArray<FVector2f> Local;
		Local.Reserve(Points.Num());
		for (const FVector2D& Point : Points)
		{
			Local.Add(FVector2f(Point.X * Size.X, Point.Y * Size.Y));
		}
		FLinearColor Tint = Color;
		Tint.A *= Style.GetColorAndOpacityTint().A;
		CoreKitBasicUIHudDraw::FillPolygon(Out, Layer, Geometry, Local, Tint);
		return Layer;
	}
};

TSharedRef<SWidget> UCoreKitBasicUIHudShape::RebuildWidget()
{
	Shape = SNew(SCoreKitBasicUIHudShape);
	return Shape.ToSharedRef();
}

void UCoreKitBasicUIHudShape::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Shape.Reset();
}

void UCoreKitBasicUIHudShape::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (Shape)
	{
		Shape->Points = Points;
		Shape->Color = ShapeColor;
		Shape->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UCoreKitBasicUIHudShape::SetPoints(const TArray<FVector2D>& InPoints)
{
	Points = InPoints;
	SynchronizeProperties();
}

void UCoreKitBasicUIHudShape::SetShapeColor(const FLinearColor& InColor)
{
	ShapeColor = InColor;
	SynchronizeProperties();
}

// ---------------------------------------------------------------------------------------------------------------
// Minimap

class SCoreKitBasicUIHudMinimap : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCoreKitBasicUIHudMinimap) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		LabelFont = UCoreKitBasicUIStyle::Font(ECoreKitBasicUIFontFamily::Mono, 700, CoreKitBasicUIBuild::Px(9.f), 0.f);
	}

	FCoreKitBasicUIMinimapPaintData Data;
	FSlateBrush PlanBrush;
	FSlateFontInfo LabelFont;

	FVector2D ComputeDesiredSize(float) const override { return FVector2D(200.f, 200.f); }

	int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out, int32 Layer,
		const FWidgetStyle& Style, bool) const override
	{
		using namespace CoreKitBasicUIHudDraw;
		const float K = Geometry.GetLocalSize().X / 200.f;
		const float Alpha = Style.GetColorAndOpacityTint().A;
		auto C = [Alpha](FLinearColor Color) { Color.A *= Alpha; return Color; };
		auto P = [K](float X, float Y) { return FVector2f(X * K, Y * K); };
		const float Frame = 168.f;
		auto Map = [&](const FVector2D& World)
		{
			const FVector2D M = CoreKitBasicUIHudLogic::WorldToMap(World, Data.BoundsMin, Data.BoundsMax, Frame);
			return P(16.f + M.X, 16.f + M.Y);
		};
		const float LenK = CoreKitBasicUIHudLogic::WorldLengthToMap(1.f, Data.BoundsMin, Data.BoundsMax, Frame) * K;

		// Plate: Soot 0.62 with the 14 px chamfer, 1 px hairline.
		const TArray<FVector2f> Plate = { P(14, 0), P(200, 0), P(200, 200), P(0, 200), P(0, 14) };
		FillPolygon(Out, Layer, Geometry, Plate, C(CoreKitBasicUIColor::SootA(0.62f)));
		TArray<FVector2f> Outline = Plate;
		Outline.Add(Plate[0]);
		CoreKitBasicUISafeDraw::Lines(Out, Layer + 1, Geometry, Outline, C(CoreKitBasicUIColor::WhiteA(0.22f)), 1.f * K);

		// Plan (static capture of the arena) inside the frame; without one the plate stays plain.
		if (Data.Plan)
		{
			const FVector2f TopLeft = Map(FVector2D(Data.BoundsMax.X, Data.BoundsMin.Y));
			const FVector2f BottomRight = Map(FVector2D(Data.BoundsMin.X, Data.BoundsMax.Y));
			FSlateDrawElement::MakeBox(Out, Layer + 1, Geometry.ToPaintGeometry(BottomRight - TopLeft, FSlateLayoutTransform(TopLeft)), &PlanBrush, ESlateDrawEffect::None, C(FLinearColor::White));
		}
		StrokeRect(Out, Layer + 2, Geometry, P(16, 16), P(Frame, Frame), C(CoreKitBasicUIColor::WhiteA(0.35f)), 2.f * K);

		// Slam area of the boss.
		if (Data.AreaRadius > 0.f)
		{
			const FVector2f Center = Map(Data.AreaCenter);
			const float Radius = Data.AreaRadius * LenK;
			FillCircle(Out, Layer + 3, Geometry, Center, Radius, C(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.18f)), 28);
			StrokeCircle(Out, Layer + 4, Geometry, Center, Radius, C(CoreKitBasicUIColor::Orange), 1.5f * K, 32);
		}

		// View cone: 60 degrees, 62 px, white 0.12.
		const FVector2f Player = Map(Data.PlayerPosition);
		{
			TArray<FVector2f> Cone = { Player };
			for (int32 Step = 0; Step <= 8; ++Step)
			{
				const float Deg = Data.PlayerYaw - 30.f + 60.f * Step / 8.f;
				const float Rad = FMath::DegreesToRadians(Deg);
				Cone.Add(Player + FVector2f(FMath::Sin(Rad), -FMath::Cos(Rad)) * 62.f * K);
			}
			FillPolygon(Out, Layer + 3, Geometry, Cone, C(CoreKitBasicUIColor::WhiteA(0.12f)));
		}

		// Entities.
		const int32 EntityLayer = Layer + 5;
		bool bBossDrawn = false;
		auto DrawBoss = [&](const FVector2f& At)
		{
			CoreKitBasicUIHudDraw::Rect(Out, EntityLayer + 1, Geometry, At - P(12, 12), P(24, 24), C(CoreKitBasicUIColor::Orange));
			StrokeRect(Out, EntityLayer + 1, Geometry, At - P(18, 18), P(36, 36), C(CoreKitBasicUIColor::Orange), 2.f * K);
		};
		for (const FCoreKitBasicUIMinimapEntity& Entity : Data.Entities)
		{
			const FVector2f At = Map(Entity.Position);
			switch (Entity.Kind)
			{
			case ECoreKitBasicUIMinimapKind::DroneSphere:
				FillCircle(Out, EntityLayer, Geometry, At, 4.f * K, C(CoreKitBasicUIColor::Orange), 12);
				break;
			case ECoreKitBasicUIMinimapKind::DroneOther:
				FillCircle(Out, EntityLayer, Geometry, At, 3.5f * K, C(CoreKitBasicUIColor::Orange), 12);
				break;
			case ECoreKitBasicUIMinimapKind::DroneCube:
				CoreKitBasicUIHudDraw::Rect(Out, EntityLayer, Geometry, At - P(4.5f, 4.5f), P(9, 9), C(CoreKitBasicUIColor::Orange));
				break;
			case ECoreKitBasicUIMinimapKind::DronePyramid:
				FillPolygon(Out, EntityLayer, Geometry, { At + P(0, -5), At + P(5, 4), At + P(-5, 4) }, C(CoreKitBasicUIColor::Orange));
				break;
			case ECoreKitBasicUIMinimapKind::Boss:
				DrawBoss(At);
				bBossDrawn = true;
				break;
			case ECoreKitBasicUIMinimapKind::XPOrb:
				FillPolygon(Out, EntityLayer - 1, Geometry, { At + P(0, -3), At + P(3, 0), At + P(0, 3), At + P(-3, 0) }, C(CoreKitBasicUIColor::Lime));
				break;
			case ECoreKitBasicUIMinimapKind::RepairKit:
				CoreKitBasicUIHudDraw::Rect(Out, EntityLayer - 1, Geometry, At - P(2, 6), P(4, 12), C(CoreKitBasicUIColor::Mint));
				CoreKitBasicUIHudDraw::Rect(Out, EntityLayer - 1, Geometry, At - P(6, 2), P(12, 4), C(CoreKitBasicUIColor::Mint));
				break;
			}
			if (Entity.bAlert)
			{
				// Alert ring: 8-9 px radius, 1.5 px, pulsing (03-HUDEvents #15).
				const float Radius = (8.f + Data.Pulse) * K;
				StrokeCircle(Out, EntityLayer + 2, Geometry, At, Radius, C(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.55f + 0.45f * Data.Pulse)), 1.5f * K, 24);
			}
		}
		if (!bBossDrawn && Data.bBossVisible)
		{
			DrawBoss(Map(Data.BossPosition));
		}

		// Player arrow (white with a Soot outline), rotated with the yaw.
		{
			const FVector2f Offsets[] = { P(0, -8), P(7, 8), P(0, 4), P(-7, 8) };
			TArray<FVector2f> Arrow;
			for (const FVector2f& Offset : Offsets)
			{
				Arrow.Add(Rotate(Player + Offset, Player, Data.PlayerYaw));
			}
			FillPolygon(Out, EntityLayer + 3, Geometry, Arrow, C(CoreKitBasicUIColor::White));
			const FVector2f First = Arrow[0];
			Arrow.Add(First);
			CoreKitBasicUISafeDraw::Lines(Out, EntityLayer + 4, Geometry, Arrow, C(CoreKitBasicUIColor::Soot), 1.f * K);
		}

		// North mark.
		{
			FSlateFontInfo Font = LabelFont;
			Font.Size *= K;
			FVector2f TextSize(6.f * K, 10.f * K);
			if (FSlateApplication::IsInitialized())
			{
				TextSize = FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT("N"), Font));
			}
			const FVector2f At = P(100.f, 12.f) - FVector2f(TextSize.X * 0.5f, TextSize.Y * 0.78f);
			FSlateDrawElement::MakeText(Out, EntityLayer + 5, Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(At)), FString(TEXT("N")), Font, ESlateDrawEffect::None, C(CoreKitBasicUIColor::KeyLight));
		}
		return EntityLayer + 5;
	}
};

TSharedRef<SWidget> UCoreKitBasicUIMinimapView::RebuildWidget()
{
	Map = SNew(SCoreKitBasicUIHudMinimap);
	SetPaintData(Data);
	return Map.ToSharedRef();
}

void UCoreKitBasicUIMinimapView::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Map.Reset();
}

void UCoreKitBasicUIMinimapView::SetPaintData(const FCoreKitBasicUIMinimapPaintData& InData)
{
	Data = InData;
	if (Map)
	{
		if (Map->Data.Plan != Data.Plan)
		{
			Map->PlanBrush = FSlateBrush();
			if (Data.Plan)
			{
				Map->PlanBrush.SetResourceObject(Data.Plan);
				Map->PlanBrush.ImageSize = FVector2D(Data.Plan->GetSizeX(), Data.Plan->GetSizeY());
				Map->PlanBrush.DrawAs = ESlateBrushDrawType::Image;
			}
		}
		Map->Data = Data;
		Map->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// HUD keycap

TSharedRef<SWidget> UCoreKitBasicUIHudKeycap::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("Root"));
		SizeBox = CoreKitBasicUIBuild::Box(WidgetTree, Root, 0.f, Height, TEXT("Size"));
		SizeBox->SetMinDesiredWidth(Height);
		WidgetTree->RootWidget = SizeBox;
		Edge = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(BorderColor), TEXT("Edge"));
		CoreKitBasicUIBuild::AddO(Root, Edge);
		Face = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.6f)), FMargin(6.f, 0.f), TEXT("Face"));
		Face->SetHorizontalAlignment(HAlign_Center);
		Face->SetVerticalAlignment(VAlign_Center);
		CoreKitBasicUIBuild::AddO(Root, Face, HAlign_Fill, VAlign_Fill, FMargin(BorderPx, BorderPx, BorderPx, BottomPx));
		Label = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(LabelPx), 700, TextColor, 0.f), TEXT("Label"));
		Face->SetContent(Label);
		Icon = CoreKitBasicUIBuild::Image(WidgetTree, FSlateBrush(), TEXT("Icon"));
		CoreKitBasicUIBuild::AddO(Root, Icon, HAlign_Center, VAlign_Center);
		ApplyLook();
	}
	return UUserWidget::RebuildWidget();
}

void UCoreKitBasicUIHudKeycap::SetAccent(const FLinearColor& InBorder, const FLinearColor& InText)
{
	BorderColor = InBorder;
	TextColor = InText;
	ApplyLook();
}

void UCoreKitBasicUIHudKeycap::SetMetrics(float InHeight, float InLabelPx, float InBorder, float InBottom)
{
	Height = InHeight;
	LabelPx = InLabelPx;
	BorderPx = InBorder;
	BottomPx = InBottom;
	ApplyLook();
}

void UCoreKitBasicUIHudKeycap::ApplyLook()
{
	if (!Edge)
	{
		return;
	}
	const bool bHasIcon = Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed && Icon->GetBrush().GetResourceObject() != nullptr;
	const bool bHasLabel = Label && Label->GetVisibility() != ESlateVisibility::Collapsed && !Label->GetText().IsEmpty();
	Edge->SetVisibility(bHasIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	Face->SetVisibility(bHasIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (SizeBox)
	{
		// Nothing to show (no key and no fallback): no empty frame.
		SizeBox->SetVisibility(bHasIcon || bHasLabel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	Edge->SetColorAndOpacity(BorderColor);
	if (UOverlaySlot* FaceSlot = Cast<UOverlaySlot>(Face->Slot))
	{
		FaceSlot->SetPadding(FMargin(BorderPx, BorderPx, BorderPx, BottomPx));
	}
	if (SizeBox)
	{
		SizeBox->SetHeightOverride(Height);
		SizeBox->SetMinDesiredWidth(Height);
	}
	if (Label)
	{
		CoreKitBasicUIBuild::ApplyTextStyle(Label, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(LabelPx), 700, TextColor, 0.f));
	}
	if (Icon)
	{
		FSlateBrush Brush = Icon->GetBrush();
		Brush.ImageSize = FVector2D(Height + 2.f, Height + 2.f);
		Icon->SetBrush(Brush);
	}
}

void UCoreKitBasicUIHudKeycap::SetFallbackKeys(FKey InKeyboard, FKey InGamepad)
{
	FallbackKeyboard = InKeyboard;
	FallbackGamepad = InGamepad;
	Refresh();
}

void UCoreKitBasicUIHudKeycap::NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey)
{
	// The action has no key for this device (unmapped or not registered yet): show the spec default key instead of an
	// empty frame (keyboard text keycap, gamepad glyph).
	if (!bHasKey && (FallbackKeyboard.IsValid() || FallbackGamepad.IsValid()))
	{
		const ULocalPlayer* Player = GetOwningLocalPlayer();
		const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
		const bool bGamepad = Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad;
		const FKey Fallback = bGamepad && FallbackGamepad.IsValid() ? FallbackGamepad : FallbackKeyboard;
		if (Nav && Fallback.IsValid())
		{
			const FCoreKitKeyPresentation Shown = Nav->FindKeyPresentation(Fallback);
			UTexture2D* Texture = Shown.Icon.LoadSynchronous();
			if (Icon)
			{
				Icon->SetBrushFromTexture(Texture, false);
				Icon->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			}
			if (Label)
			{
				Label->SetText(Shown.DisplayName);
				Label->SetVisibility(!Texture && !Shown.DisplayName.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			}
		}
	}
	ApplyLook();
}

// ---------------------------------------------------------------------------------------------------------------
// Utilities

UInputAction* CoreKitBasicUIHudUtil::FindInputAction(const TCHAR* Name)
{
	return GetDefault<UCoreKitBasicUISettings>()->FindAction(FName(Name));
}

void CoreKitBasicUIHudUtil::SetAccessibleText(UWidget* Widget, const FText& Text)
{
#if WITH_ACCESSIBILITY
	if (Widget)
	{
		if (TSharedPtr<SWidget> Slate = Widget->GetCachedWidget())
		{
			Slate->SetAccessibleBehavior(EAccessibleBehavior::Custom, Text);
		}
	}
#endif
}

float CoreKitBasicUIHudUtil::MeasureText(const FText& Text, const FSlateFontInfo& Font)
{
	if (!FSlateApplication::IsInitialized())
	{
		return Text.ToString().Len() * Font.Size * 0.6f;
	}
	return static_cast<float>(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font).X);
}
