#pragma once

#include "Fonts/SlateFontInfo.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "CoreKitBasicUIStyle.generated.h"

class UTexture2D;

/** Design system I "Rooftop Works" colour tokens (Docs/UI/Spec/01-DesignSystem.md). sRGB hex converted to linear. */
namespace CoreKitBasicUIColor
{
	inline FLinearColor Hex(uint32 RGB, float Alpha = 1.f)
	{
		FLinearColor C = FLinearColor(FColor((RGB >> 16) & 0xFF, (RGB >> 8) & 0xFF, RGB & 0xFF, 255));
		C.A = Alpha;
		return C;
	}
	inline const FLinearColor Magenta = Hex(0xFF3FD2);
	inline const FLinearColor MagentaDark = Hex(0xA01488);
	inline const FLinearColor White = Hex(0xF2F3F5);
	inline const FLinearColor Soot = Hex(0x0A0A0C);
	inline const FLinearColor Hull = Hex(0x141418);
	inline const FLinearColor Plate = Hex(0x222328);
	inline const FLinearColor Seam = Hex(0x3A3D44);
	inline const FLinearColor Steel = Hex(0x9A9DA5);
	inline const FLinearColor KeyLight = Hex(0xE6E8EC);
	inline const FLinearColor Muted = Hex(0x8E9199);
	inline const FLinearColor Shade = Hex(0x5A5D66);
	inline const FLinearColor Divider = Hex(0x2A2C31);
	inline const FLinearColor TextDim = Hex(0xB9BCC3);
	inline const FLinearColor TextSoft = Hex(0xC9CCD2);
	inline const FLinearColor Orange = Hex(0xFF7A1A);
	inline const FLinearColor Lime = Hex(0xB8FF3A);
	inline const FLinearColor Mint = Hex(0x4DFFC3);
	inline const FLinearColor Red = Hex(0xFF3B4E);
	inline const FLinearColor Transparent = FLinearColor(0.f, 0.f, 0.f, 0.f);
	/** Soot with alpha, for HUD plates and scrims. */
	inline FLinearColor SootA(float Alpha) { return Hex(0x0A0A0C, Alpha); }
	inline FLinearColor WhiteA(float Alpha) { return Hex(0xF2F3F5, Alpha); }
	inline FLinearColor WithAlpha(const FLinearColor& Color, float Alpha) { FLinearColor C = Color; C.A = Alpha; return C; }
}

UENUM(BlueprintType)
enum class ECoreKitBasicUIFontFamily : uint8
{
	/** Saira Condensed: titles, HUD numbers, card names, buttons. */
	Display,
	/** Saira Stencil One: logo. */
	Stencil,
	/** Barlow Semi Condensed: body text. */
	Body,
	/** JetBrains Mono: labels, keycaps, data. */
	Mono
};

UENUM(BlueprintType)
enum class ECoreKitBasicUICardSuit : uint8
{
	Sight,
	Edge,
	Vector,
	Plate
};

/** Fonts, brushes and textures of the design system. Assets live in /CoreKit/BasicUI. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIStyle : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Weight 400..800 (snapped to the family's available weights). Tracking in em (0.2 = 0.2 em). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateFontInfo Font(ECoreKitBasicUIFontFamily Family, int32 Weight, float Size, float TrackingEm = 0.f);

	/** Texture from /CoreKit/BasicUI/Textures by name (for example T_UI_Chamfer14, T_Icon_Skull). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static UTexture2D* Texture(FName Name);

	/** Plain image brush tinted with Color. Size zero keeps the texture size. */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateBrush Image(FName TextureName, FLinearColor Color, FVector2D Size = FVector2D::ZeroVector);

	/** Solid colour brush. */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateBrush Solid(FLinearColor Color);

	/** Panel with the design system's 45-degree top-left chamfer of Cut px (6, 7, 8, 10, 11, 12, 14, 16, 18, 20, 22, 24). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateBrush Chamfer(int32 Cut, FLinearColor Color);

	/** Bottom-right 26 px chamfer (logo DRAW plate). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateBrush ChamferBottomRight(FLinearColor Color);

	/** Hazard tape: tiled 45-degree stripes of StripeWidth px in Color over a transparent gap (put a Soot panel behind). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateBrush Tape(int32 StripeWidth, FLinearColor Color);

	/** Tiled scanlines at the given alpha. */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FSlateBrush Scanlines(float Alpha);

	/** Accent colour of a suit (SIGHT white, EDGE/VECTOR magenta, PLATE mint). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FLinearColor SuitColor(ECoreKitBasicUICardSuit Suit);

	/** Glyph texture name of a suit (T_Icon_Sight...). */
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI|Style")
	static FName SuitIcon(ECoreKitBasicUICardSuit Suit);

	/** Button style from the design system (Default / Hover / Focus colours) on a chamfered plate. */
	static FButtonStyle ButtonStyle(int32 Cut, const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed);

	/** Transparent button style (content draws itself). */
	static FButtonStyle ClearButtonStyle();
};
