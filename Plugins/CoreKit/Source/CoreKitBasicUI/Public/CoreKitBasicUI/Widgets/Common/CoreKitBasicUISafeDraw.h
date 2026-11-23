#pragma once

#include "CoreMinimal.h"
#include "Rendering/DrawElements.h"

/**
 * Guarded Slate line drawing. Slate asserts (RenderBatch.NumIndices > 0, SlateRHIRenderingPolicy) when a line batch
 * ends up with no geometry: every point on the same spot (zero-radius ring, zero-size widget) or non-finite points
 * (a projected world ring at the edge of the view). These helpers drop non-finite and repeated points and skip the
 * draw when no real segment remains. Use them for every runtime-sized polyline in the UI.
 */
namespace CoreKitBasicUISafeDraw
{
	template <typename VectorType>
	bool CleanPolyline(const TArray<VectorType>& In, TArray<VectorType>& Out)
	{
		Out.Reset(In.Num());
		for (const VectorType& Point : In)
		{
			if (!FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y))
			{
				return false;
			}
			if (Out.Num() == 0 || FVector2D(Point - Out.Last()).SizeSquared() > 1.e-4)
			{
				Out.Add(Point);
			}
		}
		return Out.Num() >= 2;
	}

	template <typename VectorType>
	void Lines(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const TArray<VectorType>& Points,
		const FLinearColor& Color, float Thickness, bool bAntialias = true)
	{
		if (Color.A <= 0.f || !FMath::IsFinite(Thickness) || Thickness <= 0.f)
		{
			return;
		}
		const FVector2D Size = Geometry.GetLocalSize();
		if (!FMath::IsFinite(Size.X) || !FMath::IsFinite(Size.Y))
		{
			return;
		}
		TArray<VectorType> Clean;
		if (!CleanPolyline(Points, Clean))
		{
			return;
		}
		FSlateDrawElement::MakeLines(Out, Layer, Geometry.ToPaintGeometry(), Clean, ESlateDrawEffect::None, Color, bAntialias, Thickness);
	}
}
