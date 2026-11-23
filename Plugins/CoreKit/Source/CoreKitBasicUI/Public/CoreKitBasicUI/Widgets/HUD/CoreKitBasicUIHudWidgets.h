#pragma once

#include "Components/Widget.h"
#include "CoreKitNavigation/CoreKitInputPrompt.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIHudWidgets.generated.h"

class SCoreKitBasicUIHudShape;
class SCoreKitBasicUIHudMinimap;
class UBorder;
class UInputAction;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * Base of the HUD blocks: Reduced motion helper. Blocks read their ViewModels in TickMotion, which UCoreKitBasicUIHudRoot calls
 * every frame through TickHud (so a block keeps ticking while it collapses itself; Slate does not tick collapsed
 * widgets). The automatic NativeTick -> TickMotion of UCoreKitBasicUIUserWidget is disabled here to avoid ticking twice.
 */
UCLASS(Abstract)
class COREKITBASICUI_API UCoreKitBasicUIHudBlock : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	/** Real-time step driven by UCoreKitBasicUIHudRoot (also while paused). */
	void TickHud(float RealDeltaSeconds);

protected:
	void NativeTick(const FGeometry& Geometry, float DeltaTime) override { UUserWidget::NativeTick(Geometry, DeltaTime); }

	/** Reduced motion (global flag or Preferences ViewModel): no shake, jitter, speed lines or travel; fades stay. */
	bool IsReducedMotion() const;
	/** 1, or 0 under Reduced motion: multiplies travel / scale pops. */
	float MotionAmount() const { return IsReducedMotion() ? 0.f : 1.f; }
	void PlayHudCue(FName Cue) const;
};

/**
 * Filled convex polygon (points normalised 0..1 in the widget box), drawn natively. Used for plates whose shape is
 * not a chamfer (lock-on plate with a point).
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudShape : public UWidget
{
	GENERATED_BODY()

public:
	void SetPoints(const TArray<FVector2D>& InPoints);
	void SetShapeColor(const FLinearColor& InColor);

	void SynchronizeProperties() override;
	void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, Category = "Shape") TArray<FVector2D> Points;
	UPROPERTY(EditAnywhere, Category = "Shape") FLinearColor ShapeColor = FLinearColor::White;

private:
	TSharedPtr<SCoreKitBasicUIHudShape> Shape;
};

/** Everything the minimap paints in one frame (02-HUD § Minimapa). */
USTRUCT()
struct COREKITBASICUI_API FCoreKitBasicUIMinimapPaintData
{
	GENERATED_BODY()

	UPROPERTY(Transient) TObjectPtr<UTexture2D> Plan;
	FVector2D BoundsMin = FVector2D(-2500.f, -2500.f);
	FVector2D BoundsMax = FVector2D(2500.f, 2500.f);
	FVector2D PlayerPosition = FVector2D::ZeroVector;
	float PlayerYaw = 0.f;
	TArray<FCoreKitBasicUIMinimapEntity> Entities;
	/** Boss marker fallback when no Boss entity is listed. */
	bool bBossVisible = false;
	FVector2D BossPosition = FVector2D::ZeroVector;
	/** Slam area (radius 0 = none). */
	FVector2D AreaCenter = FVector2D::ZeroVector;
	float AreaRadius = 0.f;
	/** PULSE 0..1 for the alert rings. */
	float Pulse = 0.f;
};

/**
 * 200 x 200 minimap drawn natively in one leaf widget: chamfered plate, 168 px arena frame, plan texture (or a plain
 * plate when there is none), view cone, drones by shape, XP orbs, repair kits, alert rings, boss marker and slam area,
 * player arrow and the "N" mark. North (+X) up.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIMinimapView : public UWidget
{
	GENERATED_BODY()

public:
	void SetPaintData(const FCoreKitBasicUIMinimapPaintData& InData);
	const FCoreKitBasicUIMinimapPaintData& GetPaintData() const { return Data; }

	void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient) FCoreKitBasicUIMinimapPaintData Data;

private:
	TSharedPtr<SCoreKitBasicUIHudMinimap> Map;
};

/**
 * HUD keycap (.kc of the design system): 20 px, 1 px border with a 3 px bottom edge, Soot 0.6 face, JetBrains Mono
 * 10 px. Shows the current binding of an action (gamepad glyph on pads) and can be tinted (magenta "R").
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHudKeycap : public UCoreKitInputPrompt
{
	GENERATED_BODY()

public:
	void SetAccent(const FLinearColor& InBorder, const FLinearColor& InText);
	/** Height in px (20 in the HUD, 30 for the LOW AMMO warning) and label size in CSS px. */
	void SetMetrics(float InHeight, float InLabelPx, float InBorder = 1.f, float InBottom = 3.f);
	/** Keys shown when the action has no key for the active device (keyboard/mouse, gamepad); the spec default. */
	void SetFallbackKeys(FKey InKeyboard, FKey InGamepad);

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey) override;

private:
	void ApplyLook();

	UPROPERTY(Transient) TObjectPtr<UImage> Edge;
	UPROPERTY(Transient) TObjectPtr<UBorder> Face;
	UPROPERTY(Transient) TObjectPtr<class USizeBox> SizeBox;

	FKey FallbackKeyboard;
	FKey FallbackGamepad;
	FLinearColor BorderColor = CoreKitBasicUIColor::TextSoft;
	FLinearColor TextColor = CoreKitBasicUIColor::White;
	float Height = 20.f;
	float LabelPx = 10.f;
	float BorderPx = 1.f;
	float BottomPx = 3.f;
};

namespace CoreKitBasicUIHudUtil
{
	/** Loads an input action from /Game/CoreKitBasicUI/Input (nullptr when it does not exist yet). */
	COREKITBASICUI_API UInputAction* FindInputAction(const TCHAR* Name);
	/** Sets the accessible text of a constructed widget where the platform supports it. */
	COREKITBASICUI_API void SetAccessibleText(UWidget* Widget, const FText& Text);
	/** Width of a text in Slate units for a font. */
	COREKITBASICUI_API float MeasureText(const FText& Text, const FSlateFontInfo& Font);
}
