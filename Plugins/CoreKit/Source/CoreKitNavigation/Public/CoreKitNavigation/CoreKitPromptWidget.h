#pragma once

#include "CoreKitNavigation/CoreKitMenuWidget.h"
#include "CoreKitPromptWidget.generated.h"

class UCoreKitButton;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_DELEGATE_OneParam(FCoreKitPromptCallback, int32, Option);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitPromptDecided, int32, Option);

/**
 * Modal prompt: title, body and N options. Return chooses CancelOption. The decision is reported once,
 * then the prompt closes. Subclass it for a styled layout with "Title", "Body" and an "OptionPanel" panel.
 */
UCLASS(meta = (DisplayName = "Core Kit Prompt Widget"))
class COREKITNAVIGATION_API UCoreKitPromptWidget : public UCoreKitMenuWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetupPrompt(const FText& InTitle, const FText& InBody, const TArray<FText>& InOptions, int32 InCancelOption, int32 InDefaultOption);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void Decide(int32 Option);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	bool HasDecided() const { return bDecided; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	const TArray<FText>& GetOptions() const { return Options; }

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitPromptDecided OnPromptDecided;

	FCoreKitPromptCallback Callback;

	/** Lets a project style each generated option button. */
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FStyleOption, UCoreKitButton* /*Button*/, int32 /*Index*/, bool /*bIsCancel*/);
	FStyleOption OnStyleOption;

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;

	/** Hook for subclasses after the default content is filled. */
	virtual void NativeOnPromptSetup() {}

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Title;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Body;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> OptionPanel;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CoreKit|Navigation")
	TArray<TObjectPtr<UCoreKitButton>> OptionButtons;

	FText PromptTitle;
	FText PromptBody;
	TArray<FText> Options;
	int32 CancelOption = 0;
	int32 DefaultOption = 0;

private:
	void RebuildOptions();

	UFUNCTION()
	void HandleOptionActivated(UCoreKitButton* Button);

	bool bDecided = false;
};
