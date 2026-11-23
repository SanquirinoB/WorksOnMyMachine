#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"

#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"

namespace
{
	struct FFamilyFaces
	{
		const TCHAR* Prefix;
		TArray<TPair<int32, const TCHAR*>> Weights;
	};

	const FFamilyFaces& Faces(ECoreKitBasicUIFontFamily Family)
	{
		static const FFamilyFaces Display{ TEXT("SairaCondensed"), { {400, TEXT("Regular")}, {500, TEXT("Medium")}, {600, TEXT("SemiBold")}, {700, TEXT("Bold")}, {800, TEXT("ExtraBold")} } };
		static const FFamilyFaces Stencil{ TEXT("SairaStencilOne"), { {400, TEXT("Regular")} } };
		static const FFamilyFaces Body{ TEXT("BarlowSemiCondensed"), { {400, TEXT("Regular")}, {500, TEXT("Medium")}, {600, TEXT("SemiBold")}, {700, TEXT("Bold")} } };
		static const FFamilyFaces Mono{ TEXT("JetBrainsMono"), { {400, TEXT("Regular")}, {500, TEXT("Medium")}, {700, TEXT("Bold")} } };
		switch (Family)
		{
		case ECoreKitBasicUIFontFamily::Stencil: return Stencil;
		case ECoreKitBasicUIFontFamily::Body: return Body;
		case ECoreKitBasicUIFontFamily::Mono: return Mono;
		default: return Display;
		}
	}

	const TCHAR* SnapWeight(const FFamilyFaces& FamilyFaces, int32 Weight)
	{
		const TCHAR* Best = FamilyFaces.Weights[0].Value;
		int32 BestDistance = MAX_int32;
		for (const TPair<int32, const TCHAR*>& Entry : FamilyFaces.Weights)
		{
			const int32 Distance = FMath::Abs(Entry.Key - Weight);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Entry.Value;
			}
		}
		return Best;
	}

	/** One runtime composite font per family, with one typeface entry per weight. */
	TSharedPtr<const FCompositeFont> CompositeFor(ECoreKitBasicUIFontFamily Family)
	{
		static TMap<uint8, TSharedPtr<const FCompositeFont>> Cache;
		static TArray<TStrongObjectPtr<UFontFace>> Keep;
		if (const TSharedPtr<const FCompositeFont>* Found = Cache.Find(static_cast<uint8>(Family)))
		{
			return *Found;
		}
		const FFamilyFaces& FamilyFaces = Faces(Family);
		TSharedRef<FCompositeFont> Composite = MakeShared<FCompositeFont>();
		for (const TPair<int32, const TCHAR*>& Entry : FamilyFaces.Weights)
		{
			const FString Name = FString::Printf(TEXT("%s-%s"), FamilyFaces.Prefix, Entry.Value);
			const FString Path = FString::Printf(TEXT("/CoreKit/BasicUI/Fonts/%s.%s"), *Name, *Name);
			if (UFontFace* Face = LoadObject<UFontFace>(nullptr, *Path))
			{
				Keep.Emplace(Face);
				Composite->DefaultTypeface.Fonts.Add(FTypefaceEntry(FName(Entry.Value)));
				Composite->DefaultTypeface.Fonts.Last().Font = FFontData(Face);
			}
		}
		Cache.Add(static_cast<uint8>(Family), Composite);
		return Composite;
	}
}

FSlateFontInfo UCoreKitBasicUIStyle::Font(ECoreKitBasicUIFontFamily Family, int32 Weight, float Size, float TrackingEm)
{
	FSlateFontInfo Info(CompositeFor(Family), Size, FName(SnapWeight(Faces(Family), Weight)));
	Info.LetterSpacing = FMath::RoundToInt(TrackingEm * 1000.f);
	return Info;
}

UTexture2D* UCoreKitBasicUIStyle::Texture(FName Name)
{
	static TMap<FName, TStrongObjectPtr<UTexture2D>> Cache;
	if (const TStrongObjectPtr<UTexture2D>* Found = Cache.Find(Name))
	{
		return Found->Get();
	}
	const FString Path = FString::Printf(TEXT("/CoreKit/BasicUI/Textures/%s.%s"), *Name.ToString(), *Name.ToString());
	UTexture2D* Loaded = LoadObject<UTexture2D>(nullptr, *Path);
	Cache.Add(Name, TStrongObjectPtr<UTexture2D>(Loaded));
	return Loaded;
}

FSlateBrush UCoreKitBasicUIStyle::Image(FName TextureName, FLinearColor Color, FVector2D Size)
{
	FSlateBrush Brush;
	if (UTexture2D* Tex = Texture(TextureName))
	{
		Brush.SetResourceObject(Tex);
		// Imported size, not GetSizeX/Y: in editor builds (-game captures, PIE) a texture still compiling reports the
		// 32 x 32 default texture, which gave auto-sized brushes a wrong size (a 4 x 256 ramp became 32 x 32).
		const FIntPoint Imported = Tex->GetImportedSize();
		const FVector2D TextureSize = Imported.X > 0 && Imported.Y > 0 ? FVector2D(Imported) : FVector2D(Tex->GetSizeX(), Tex->GetSizeY());
		Brush.ImageSize = Size.IsNearlyZero() ? TextureSize : Size;
	}
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Color);
	return Brush;
}

FSlateBrush UCoreKitBasicUIStyle::Solid(FLinearColor Color)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.TintColor = FSlateColor(Color);
	Brush.SetResourceObject(Texture(TEXT("T_UI_White")));
	Brush.Margin = FMargin(0.f);
	return Brush;
}

FSlateBrush UCoreKitBasicUIStyle::Chamfer(int32 Cut, FLinearColor Color)
{
	static const int32 Available[] = { 6, 7, 8, 10, 11, 12, 14, 16, 18, 20, 22, 24 };
	int32 Best = Available[0];
	for (int32 Value : Available)
	{
		if (FMath::Abs(Value - Cut) < FMath::Abs(Best - Cut))
		{
			Best = Value;
		}
	}
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture(*FString::Printf(TEXT("T_UI_Chamfer%d"), Best)));
	Brush.DrawAs = ESlateBrushDrawType::Box;
	const float M = Best / 64.f;
	Brush.Margin = FMargin(M, M, 2.f / 64.f, 2.f / 64.f);
	Brush.ImageSize = FVector2D(64.f, 64.f);
	Brush.TintColor = FSlateColor(Color);
	return Brush;
}

FSlateBrush UCoreKitBasicUIStyle::ChamferBottomRight(FLinearColor Color)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture(TEXT("T_UI_ChamferBR26")));
	Brush.DrawAs = ESlateBrushDrawType::Box;
	const float M = 26.f / 64.f;
	Brush.Margin = FMargin(2.f / 64.f, 2.f / 64.f, M, M);
	Brush.ImageSize = FVector2D(64.f, 64.f);
	Brush.TintColor = FSlateColor(Color);
	return Brush;
}

FSlateBrush UCoreKitBasicUIStyle::Tape(int32 StripeWidth, FLinearColor Color)
{
	static const int32 Available[] = { 5, 6, 8, 10, 12, 14, 16 };
	int32 Best = Available[0];
	for (int32 Value : Available)
	{
		if (FMath::Abs(Value - StripeWidth) < FMath::Abs(Best - StripeWidth))
		{
			Best = Value;
		}
	}
	FSlateBrush Brush = Image(*FString::Printf(TEXT("T_UI_Tape%d"), Best), Color);
	Brush.Tiling = ESlateBrushTileType::Both;
	return Brush;
}

FSlateBrush UCoreKitBasicUIStyle::Scanlines(float Alpha)
{
	FSlateBrush Brush = Image(TEXT("T_UI_Scanlines"), CoreKitBasicUIColor::WhiteA(Alpha));
	Brush.Tiling = ESlateBrushTileType::Both;
	return Brush;
}

FLinearColor UCoreKitBasicUIStyle::SuitColor(ECoreKitBasicUICardSuit Suit)
{
	switch (Suit)
	{
	case ECoreKitBasicUICardSuit::Sight: return CoreKitBasicUIColor::White;
	case ECoreKitBasicUICardSuit::Plate: return CoreKitBasicUIColor::Mint;
	default: return CoreKitBasicUIColor::Magenta;
	}
}

FName UCoreKitBasicUIStyle::SuitIcon(ECoreKitBasicUICardSuit Suit)
{
	switch (Suit)
	{
	case ECoreKitBasicUICardSuit::Sight: return TEXT("T_Icon_Sight");
	case ECoreKitBasicUICardSuit::Edge: return TEXT("T_Icon_Edge");
	case ECoreKitBasicUICardSuit::Vector: return TEXT("T_Icon_Vector");
	default: return TEXT("T_Icon_Plate");
	}
}

FButtonStyle UCoreKitBasicUIStyle::ButtonStyle(int32 Cut, const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed)
{
	FButtonStyle Style;
	Style.SetNormal(Chamfer(Cut, Normal));
	Style.SetHovered(Chamfer(Cut, Hovered));
	Style.SetPressed(Chamfer(Cut, Pressed));
	Style.SetDisabled(Chamfer(Cut, CoreKitBasicUIColor::WithAlpha(Normal, Normal.A * 0.55f)));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	return Style;
}

FButtonStyle UCoreKitBasicUIStyle::ClearButtonStyle()
{
	FButtonStyle Style;
	FSlateBrush None;
	None.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.SetNormal(None);
	Style.SetHovered(None);
	Style.SetPressed(None);
	Style.SetDisabled(None);
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	return Style;
}
