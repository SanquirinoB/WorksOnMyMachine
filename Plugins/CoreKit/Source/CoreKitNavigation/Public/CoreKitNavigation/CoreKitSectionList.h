#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreKitSectionList.generated.h"

class UCoreKitButton;
class UPanelWidget;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitSectionChanged, int32, SectionIndex);

/**
 * Tabs. Builds one Core Kit Button per section (or uses the buttons placed in the panel named "SectionPanel"),
 * keeps an optional Widget Switcher in sync, and responds to the owning menu's Next/Previous section input.
 */
UCLASS(meta = (DisplayName = "Core Kit Section List"))
class COREKITNAVIGATION_API UCoreKitSectionList : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetSections(const TArray<FText>& InSections);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetSwitcher(UWidgetSwitcher* InSwitcher);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetActiveSection(int32 Index);

	/** Moves by Delta with wrap-around. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void StepSection(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	int32 GetActiveSection() const { return ActiveSection; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	int32 GetSectionCount() const { return Sections.Num(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	UCoreKitButton* GetSectionButton(int32 Index) const;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitSectionChanged OnSectionChanged;

	/** Called for every generated tab so a project can style it. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FStyleTab, UCoreKitButton* /*Button*/, int32 /*Index*/, bool /*bActive*/);
	FStyleTab OnStyleTab;

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	TArray<FText> Sections;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SectionPanel;

private:
	void RebuildButtons();
	void RefreshVisuals();

	UFUNCTION()
	void HandleButtonActivated(UCoreKitButton* Button);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCoreKitButton>> Buttons;

	TWeakObjectPtr<UWidgetSwitcher> Switcher;
	int32 ActiveSection = 0;
};
