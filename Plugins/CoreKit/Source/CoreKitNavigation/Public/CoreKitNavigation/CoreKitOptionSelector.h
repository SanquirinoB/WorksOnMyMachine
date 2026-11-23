#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreKitOptionSelector.generated.h"

class UCoreKitButton;
class UPanelWidget;
class UTextBlock;

UENUM(BlueprintType)
enum class ECoreKitOptionSelectorMode : uint8
{
	/** One button per option. */
	Segmented,
	/** Previous button, current value, next button. */
	Arrows
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitOptionChanged, int32, SelectedIndex);

/**
 * Value control for settings rows. Left/right navigation (keyboard arrows, D-pad, stick) changes the value
 * while the selector or one of its buttons has focus; clicking a segment selects it.
 */
UCLASS(meta = (DisplayName = "Core Kit Option Selector"))
class COREKITNAVIGATION_API UCoreKitOptionSelector : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetOptions(const TArray<FText>& InOptions, int32 InSelected);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetSelectedIndex(int32 Index, bool bNotify = true);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void Step(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	int32 GetOptionCount() const { return Options.Num(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	UCoreKitButton* GetOptionButton(int32 Index) const;

	/** Focuses the selected segment (or the selector in Arrows mode). */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void FocusSelector();

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitOptionChanged OnValueChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	ECoreKitOptionSelectorMode Mode = ECoreKitOptionSelectorMode::Segmented;

	/** Arrows mode wraps from the last option to the first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	bool bWrap = false;

	/** Lets a project style each generated button (segment or arrow) and the value label. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FStyleOption, UCoreKitButton* /*Button*/, int32 /*Index (-1 prev, -2 next)*/, bool /*bSelected*/);
	FStyleOption OnStyleOption;

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	FNavigationReply NativeOnNavigation(const FGeometry& Geometry, const FNavigationEvent& Event, const FNavigationReply& Default) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	TArray<FText> Options;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> OptionPanel;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueLabel;

private:
	void RebuildButtons();
	void RefreshVisuals();

	UFUNCTION()
	void HandleButtonActivated(UCoreKitButton* Button);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCoreKitButton>> Buttons;

	int32 SelectedIndex = 0;
};
