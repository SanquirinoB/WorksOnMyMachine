#pragma once

#include "CoreKitNavigation/CoreKitInputPrompt.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUICommonWidgets.generated.h"

class UBorder;
class UCoreKitButton;
class UHorizontalBox;
class UImage;
class UInputAction;
class UTextBlock;

/** Number with fixed-width digit cells so timers and counters do not jitter (Slate cannot enable tabular figures). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUITabularNumber : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const CoreKitBasicUIBuild::FTextStyle& InStyle);
	void SetValue(const FString& Value);
	void SetColor(const FLinearColor& Color);
	const FString& GetValue() const { return Current; }

protected:
	void BuildTree() override;

private:
	void Rebuild();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> Cells;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> Glyphs;

	CoreKitBasicUIBuild::FTextStyle Style;
	FString Current;
	float DigitWidth = 0.f;
};

UENUM(BlueprintType)
enum class ECoreKitBasicUIButtonVariant : uint8
{
	/** Main Menu / Pause lists: Hull, hover Plate with magenta text and no inset. */
	MenuList,
	/** Design-system action: Hull; hover Plate + 6 px magenta inset; focus White + 8 px inset with Soot text. */
	Action,
	/** Primary (PLAY / RESUME): always in the focus style. */
	Primary,
	/** System confirm: magenta fill, Soot text. */
	Confirm,
	/** Destructive confirm: danger red fill, Soot text. */
	Danger,
	/** Neutral modal button (CANCEL). */
	Neutral
};

/**
 * Design-system button: chamfered plate, optional leading triangle, label and right-aligned sublabel or keycap.
 * Wraps a UCoreKitButton (single selection, cooldown, sounds).
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIActionButton : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void Setup(const FText& InLabel, const FText& InSublabel, ECoreKitBasicUIButtonVariant InVariant, float InHeight = 66.f, float InLabelPx = 30.f, bool bInTriangle = false);
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetKeyHint(UInputAction* Action, FKey Key = FKey());
	UFUNCTION(BlueprintPure, Category="CoreKit|BasicUI")
	UCoreKitButton* GetButton() const { return Button; }
	void Select();
	bool IsSelected() const;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnActivated, UCoreKitBasicUIActionButton*);
	FOnActivated OnActivated;
	FOnActivated OnSelected;

	int32 Index = INDEX_NONE;

protected:
	void BuildTree() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleActivated(class UCoreKitButton* InButton);
	UFUNCTION()
	void HandleSelected(class UCoreKitButton* InButton);
	UFUNCTION()
	void HandleDeselected(class UCoreKitButton* InButton);
	void Restyle();

	UPROPERTY(Transient) TObjectPtr<UCoreKitButton> Button;
	UPROPERTY(Transient) TObjectPtr<UImage> Plate;
	UPROPERTY(Transient) TObjectPtr<UImage> Inset;
	UPROPERTY(Transient) TObjectPtr<UImage> Triangle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Sublabel;
	UPROPERTY(Transient) TObjectPtr<class UCoreKitBasicUIKeycap> Keycap;

	FText LabelText;
	FText SublabelText;
	ECoreKitBasicUIButtonVariant Variant = ECoreKitBasicUIButtonVariant::Action;
	float Height = 66.f;
	float LabelPx = 30.f;
	bool bTriangle = false;
	bool bSelectedVisual = false;
};

/**
 * Design-system keycap that shows the player's current binding: text keycap for keyboard/mouse, gamepad glyph for
 * gamepads (CoreKitNavigation icon sets). Menu size (34-40 px) or HUD size (20 px).
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIKeycap : public UCoreKitInputPrompt
{
	GENERATED_BODY()

public:
	void SetHudSize(bool bHud) { bHudSize = bHud; }
	void SetAccent(const FLinearColor& Color);
	/** Shows fixed text instead of a binding (for example "DIR"). */
	void SetFixedText(const FText& InText);

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey) override;

private:
	void ApplyLook();

	UPROPERTY(Transient) TObjectPtr<UImage> Edge;
	UPROPERTY(Transient) TObjectPtr<UBorder> Face;

	FLinearColor Accent = CoreKitBasicUIColor::Steel;
	FText FixedText;
	bool bHudSize = false;
};
