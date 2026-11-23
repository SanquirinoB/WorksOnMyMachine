#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIArcWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUISafeDraw.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

class SCoreKitBasicUIArc : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCoreKitBasicUIArc) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	float Progress = 1.f;
	float StartDeg = 0.f;
	float SweepDeg = 360.f;
	float Thickness = 3.f;
	FLinearColor ArcColor = FLinearColor::White;
	FLinearColor TrackColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	int32 DashCount = 0;
	float Squash = 1.f;

	FVector2D ComputeDesiredSize(float) const override { return FVector2D(64.f, 64.f); }

	int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
		int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		// A newly shown/collapsing lock-on indicator can have no allocated radius.
		// Slate still creates a batch for coincident line points, then its line
		// builder emits no indices and SlateRHIRenderer asserts on that empty batch.
		if (!FMath::IsFinite(Size.X) || !FMath::IsFinite(Size.Y) || Size.X <= 0.f || Size.Y <= 0.f ||
			!FMath::IsFinite(Thickness) || Thickness <= 0.f || !FMath::IsFinite(Squash) || Squash <= 0.f)
		{
			return Layer;
		}
		const FVector2D Center = Size * 0.5f;
		const float Radius = FMath::Max(0.f, FMath::Min(Size.X, Size.Y / FMath::Max(Squash, 0.01f)) * 0.5f - Thickness * 0.5f);
		const float Opacity = Style.GetColorAndOpacityTint().A;
		if (!FMath::IsFinite(Radius) || Radius <= KINDA_SMALL_NUMBER || !FMath::IsFinite(Opacity) || Opacity <= 0.f)
		{
			return Layer;
		}
		auto Draw = [&](float From, float Sweep, const FLinearColor& Color, int32 LayerId)
		{
			if (Color.A <= 0.f || !FMath::IsFinite(From) || !FMath::IsFinite(Sweep) || Sweep <= 0.01f)
			{
				return;
			}
			const int32 Segments = FMath::Clamp(FMath::CeilToInt(Sweep / 4.f), 2, 128);
			TArray<FVector2f> Points;
			Points.Reserve(Segments + 1);
			for (int32 Index = 0; Index <= Segments; ++Index)
			{
				const float Deg = From + Sweep * Index / Segments - 90.f;
				const float Rad = FMath::DegreesToRadians(Deg);
				Points.Add(FVector2f(Center + FVector2D(FMath::Cos(Rad) * Radius, FMath::Sin(Rad) * Radius * Squash)));
			}
			FLinearColor Tinted = Color;
			Tinted.A *= Opacity;
			CoreKitBasicUISafeDraw::Lines(Out, LayerId, Geometry, Points, Tinted, Thickness);
		};
		Draw(StartDeg, SweepDeg, TrackColor, Layer);
		const float Filled = SweepDeg * FMath::Clamp(Progress, 0.f, 1.f);
		if (DashCount > 0)
		{
			const float Step = SweepDeg / DashCount;
			for (int32 Dash = 0; Dash < DashCount; ++Dash)
			{
				const float From = StartDeg + Step * Dash;
				const float Sweep = FMath::Min(Step * 0.55f, StartDeg + Filled - From);
				Draw(From, Sweep, ArcColor, Layer + 1);
			}
		}
		else
		{
			Draw(StartDeg, Filled, ArcColor, Layer + 1);
		}
		return Layer + 1;
	}
};

TSharedRef<SWidget> UCoreKitBasicUIArcWidget::RebuildWidget()
{
	Arc = SNew(SCoreKitBasicUIArc);
	return Arc.ToSharedRef();
}

void UCoreKitBasicUIArcWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Arc.Reset();
}

void UCoreKitBasicUIArcWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (Arc)
	{
		Arc->Progress = Progress;
		Arc->StartDeg = StartDeg;
		Arc->SweepDeg = SweepDeg;
		Arc->Thickness = Thickness;
		Arc->ArcColor = ArcColor;
		Arc->TrackColor = TrackColor;
		Arc->DashCount = DashCount;
		Arc->Squash = Squash;
		Arc->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UCoreKitBasicUIArcWidget::SetProgress(float InProgress) { Progress = InProgress; SynchronizeProperties(); }
void UCoreKitBasicUIArcWidget::SetArc(float InStartDeg, float InSweepDeg) { StartDeg = InStartDeg; SweepDeg = InSweepDeg; SynchronizeProperties(); }
void UCoreKitBasicUIArcWidget::SetThickness(float InThickness) { Thickness = InThickness; SynchronizeProperties(); }
void UCoreKitBasicUIArcWidget::SetArcColor(FLinearColor InColor) { ArcColor = InColor; SynchronizeProperties(); }
void UCoreKitBasicUIArcWidget::SetTrackColor(FLinearColor InColor) { TrackColor = InColor; SynchronizeProperties(); }
void UCoreKitBasicUIArcWidget::SetDashes(int32 InDashCount) { DashCount = InDashCount; SynchronizeProperties(); }
void UCoreKitBasicUIArcWidget::SetSquash(float InSquash) { Squash = InSquash; SynchronizeProperties(); }
