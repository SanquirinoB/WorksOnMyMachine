#pragma once

#include "InputCoreTypes.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUICardWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTextBlock;
class UCoreKitBasicUIKeycap;

UENUM(BlueprintType)
enum class ECoreKitBasicUICardVariant : uint8
{
	/** 360 x 540 card with scene art (Card Draw, Main Menu fan). */
	Full,
	/** 52 x 72 hand / Card Draw footer mini card: art crop, accent band, xN or star. */
	Mini,
	/** 76 x 100 Victory card: art crop, abbreviation and xN. */
	Victory,
	/** 62 x 82 Defeat card: art crop and xN, magenta accents recoloured red. */
	Defeat,
	/** Square art thumbnail (22 px TAB panel, 34 px Pause list). */
	Thumb
};

UENUM(BlueprintType)
enum class ECoreKitBasicUICardState : uint8
{
	Idle,
	Hover,
	Selected,
	Dimmed
};

/**
 * Card component of Docs/UI/Spec/06-Cards.md (owned by the overlays stage). Used by Card Draw (Full), the HUD hand,
 * Pause, Victory and Defeat (small variants). Tilt drives the two art layers in opposite directions.
 *
 * Art: FCoreKitBasicUICardData::ArtScene / ArtEffect when their textures exist; otherwise a placeholder (suit-coloured glow over a
 * dark plate plus the suit glyph) so the card still reads. Small variants crop around ArtFocus.
 * The widget's own render transform is free for the owner (deal, fly); states and tilt animate an inner root.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUICardWidget : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetVariant(ECoreKitBasicUICardVariant InVariant, float InThumbSize = 22.f);
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetCard(const FCoreKitBasicUICardData& InCard);
	/** Key hint shown on the full card (1/2/3 or the gamepad glyph); empty hides it. */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetKeyHint(const FText& InKey);
	/** Key hint from a key: shows the player's binding presentation (text keycap or gamepad glyph). Invalid hides it. */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetKeyHintKey(FKey InKey);
	/** Footer override (projection). Empty keeps the default. */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetFooter(const FText& InFooter);
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetState(ECoreKitBasicUICardState InState);
	/** -1..1 on each axis (cursor or stick); zero = flat. */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTilt(FVector2D InTilt);
	/** Glow + NEW tag (hand). */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetNew(bool bInNew);
	/** Magenta 3 px focus outline 8 px outside the card (keyboard / gamepad focus). */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetFocusOutline(bool bInVisible);
	/** Brightness of the Idle state (0.82 in Card Draw, 1 elsewhere). */
	void SetIdleBrightness(float InBrightness) { IdleBrightness = InBrightness; }

	const FCoreKitBasicUICardData& GetCard() const { return Card; }
	ECoreKitBasicUICardVariant GetVariant() const { return Variant; }
	ECoreKitBasicUICardState GetState() const { return State; }
	bool HasSceneArt() const { return bHasArt; }

	/** Layout size of a variant in px. */
	static FVector2D VariantSize(ECoreKitBasicUICardVariant InVariant, float InThumbSize = 22.f);

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;

	FCoreKitBasicUICardData Card;
	ECoreKitBasicUICardVariant Variant = ECoreKitBasicUICardVariant::Full;
	ECoreKitBasicUICardState State = ECoreKitBasicUICardState::Idle;
	FVector2D Tilt = FVector2D::ZeroVector;
	FText KeyHint;
	FText FooterOverride;
	float ThumbSize = 22.f;
	bool bNew = false;

private:
	void Rebuild();
	void BuildFull();
	void BuildSmall();
	/** Scene + effect layers (or the placeholder) into a canvas of SceneSize; Crop for small variants. */
	void BuildArt(UCanvasPanel* Target, FVector2D SceneSize, bool bCrop);
	/** Effect words into Box at FontPx; MaxWords >= 0 truncates with an ellipsis. Returns the word count. */
	int32 BuildEffectText(class UWrapBox* Box, const FLinearColor& Accent, float FontPx = 22.f, int32 MaxWords = -1);
	/** Keeps chip + name + effect above the pinned footer: one font step smaller, then ellipsis. */
	void FitText(float MaxHeight);
	void ApplyTilt();
	void ApplyStateLook();
	void ApplyKeyHint();
	FLinearColor Accent() const;
	FLinearColor FrameColor() const;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Root;
	UPROPERTY(Transient) TObjectPtr<UImage> Glow;
	UPROPERTY(Transient) TObjectPtr<UImage> FrameSolid;
	UPROPERTY(Transient) TObjectPtr<UImage> FrameTape;
	UPROPERTY(Transient) TObjectPtr<UImage> SceneLayer;
	UPROPERTY(Transient) TObjectPtr<UWidget> SceneGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> EffectGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> NameText;
	UPROPERTY(Transient) TObjectPtr<UImage> Shine;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIKeycap> Keycap;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FooterText;
	UPROPERTY(Transient) TObjectPtr<UWidget> TextColumn;
	UPROPERTY(Transient) TObjectPtr<class UWrapBox> EffectBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> NewTag;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> FocusEdges;

	FKey KeyHintKey;
	FVector2D SmoothTilt = FVector2D::ZeroVector;
	FVector2D ShineCenter = FVector2D::ZeroVector;
	FCoreKitBasicUITween Lift;
	FCoreKitBasicUITween Scale;
	FCoreKitBasicUITween Brightness;
	FCoreKitBasicUITween GlowAlpha;
	FCoreKitBasicUITween NewPop;
	float IdleBrightness = 1.f;
	float SceneOverscale = 1.06f;
	FVector2D SceneBasePos = FVector2D::ZeroVector;
	bool bFocusOutline = false;
	bool bHasArt = false;
	bool bBuilt = false;
};
