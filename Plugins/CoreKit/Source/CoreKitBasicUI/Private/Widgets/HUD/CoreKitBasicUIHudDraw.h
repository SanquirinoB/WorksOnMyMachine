#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/SlateRenderer.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUISafeDraw.h"

/** Small Slate paint helpers shared by the native HUD leaf widgets (minimap, shapes). Points are in local px. */
namespace CoreKitBasicUIHudDraw
{
	inline const FSlateBrush& WhiteBrush()
	{
		static FSlateBrush Brush = UCoreKitBasicUIStyle::Image(TEXT("T_UI_White"), FLinearColor::White);
		return Brush;
	}

	/** Filled convex polygon (triangle fan around the first point). */
	inline void FillPolygon(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const TArray<FVector2f>& Points, const FLinearColor& Color)
	{
		if (Points.Num() < 3 || Color.A <= 0.f || !FSlateApplication::IsInitialized())
		{
			return;
		}
		// Same guard as CoreKitBasicUISafeDraw: no custom-vert batch without real area (non-finite or collapsed points).
		float Area = 0.f;
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2f& A = Points[Index];
			const FVector2f& B = Points[(Index + 1) % Points.Num()];
			if (!FMath::IsFinite(A.X) || !FMath::IsFinite(A.Y))
			{
				return;
			}
			Area += A.X * B.Y - B.X * A.Y;
		}
		if (FMath::Abs(Area) < 1.e-3f)
		{
			return;
		}
		const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(WhiteBrush());
		const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
		const FColor Vertex = Color.ToFColor(true);
		TArray<FSlateVertex> Verts;
		Verts.Reserve(Points.Num());
		for (const FVector2f& Point : Points)
		{
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Point, FVector2f(0.5f, 0.5f), FVector2f(1.f, 1.f), Vertex));
		}
		TArray<SlateIndex> Indexes;
		Indexes.Reserve((Points.Num() - 2) * 3);
		for (int32 Index = 1; Index + 1 < Points.Num(); ++Index)
		{
			Indexes.Add(0);
			Indexes.Add(Index);
			Indexes.Add(Index + 1);
		}
		FSlateDrawElement::MakeCustomVerts(Out, Layer, Handle, Verts, Indexes, nullptr, 0, 0);
	}

	inline TArray<FVector2f> CirclePoints(const FVector2f& Center, float Radius, int32 Segments = 24, float SquashY = 1.f)
	{
		TArray<FVector2f> Points;
		Points.Reserve(Segments + 1);
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float A = 2.f * PI * Index / Segments;
			Points.Add(Center + FVector2f(FMath::Cos(A) * Radius, FMath::Sin(A) * Radius * SquashY));
		}
		return Points;
	}

	inline void FillCircle(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2f& Center, float Radius, const FLinearColor& Color, int32 Segments = 20)
	{
		TArray<FVector2f> Points = CirclePoints(Center, Radius, Segments);
		Points.Pop();
		FillPolygon(Out, Layer, Geometry, Points, Color);
	}

	inline void StrokeCircle(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2f& Center, float Radius, const FLinearColor& Color, float Thickness, int32 Segments = 28)
	{
		if (Color.A <= 0.f)
		{
			return;
		}
		CoreKitBasicUISafeDraw::Lines(Out, Layer, Geometry, CirclePoints(Center, Radius, Segments), Color, Thickness);
	}

	inline void Rect(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2f& Min, const FVector2f& Size, const FLinearColor& Color)
	{
		if (Color.A <= 0.f)
		{
			return;
		}
		FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Min)), &WhiteBrush(), ESlateDrawEffect::None, Color);
	}

	inline void StrokeRect(FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FVector2f& Min, const FVector2f& Size, const FLinearColor& Color, float Thickness)
	{
		if (Color.A <= 0.f)
		{
			return;
		}
		const float H = Thickness * 0.5f;
		TArray<FVector2f> Points = {
			Min + FVector2f(H, H), Min + FVector2f(Size.X - H, H), Min + FVector2f(Size.X - H, Size.Y - H), Min + FVector2f(H, Size.Y - H), Min + FVector2f(H, H) };
		CoreKitBasicUISafeDraw::Lines(Out, Layer, Geometry, Points, Color, Thickness);
	}

	/** Rotates a local point around a pivot (degrees, clockwise on screen). */
	inline FVector2f Rotate(const FVector2f& Point, const FVector2f& Pivot, float AngleDeg)
	{
		const float R = FMath::DegreesToRadians(AngleDeg);
		const float C = FMath::Cos(R);
		const float S = FMath::Sin(R);
		const FVector2f D = Point - Pivot;
		return Pivot + FVector2f(D.X * C - D.Y * S, D.X * S + D.Y * C);
	}
}
