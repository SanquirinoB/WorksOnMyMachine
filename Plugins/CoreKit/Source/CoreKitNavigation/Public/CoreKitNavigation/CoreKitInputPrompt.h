#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreKitNavigation/CoreKitInputIconSet.h"
#include "CoreKitNavigation/CoreKitNavigationTypes.h"
#include "CoreKitInputPrompt.generated.h"

class UCoreKitNavigationSubsystem;
class UEnhancedInputLocalPlayerSubsystem;
class UImage;
class UInputAction;
class UTextBlock;

/**
 * Shows the icon (or the text when there is no icon) of the key bound to an action, or of an explicit key.
 * Refreshes when the input device, the gamepad type or the Enhanced Input mappings change.
 * Use it directly (it builds an Image + TextBlock row) or subclass it with a layout that binds "Icon" and "Label".
 */
UCLASS(meta = (DisplayName = "Core Kit Input Prompt"))
class COREKITNAVIGATION_API UCoreKitInputPrompt : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetAction(UInputAction* InAction);

	/** An explicit key takes precedence over the action. Pass an invalid key to use the action again. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetKey(FKey InKey);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void Refresh();

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	FKey GetDisplayedKey() const { return DisplayedKey; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	FCoreKitKeyPresentation GetPresentation() const { return Presentation; }

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit|Navigation")
	void OnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey);

	/** Native hook called after the icon and label are updated, before OnPresentationChanged. */
	virtual void NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	TObjectPtr<UInputAction> Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	FKey Key;

	/** Show the label even when an icon exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	bool bAlwaysShowLabel = false;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UImage> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Label;

private:
	void Bind();
	void Unbind();

	UFUNCTION()
	void HandleDeviceChanged(ECoreKitInputDevice Device);

	UFUNCTION()
	void HandleGamepadTypeChanged(FName GamepadType);

	UFUNCTION()
	void HandleMappingsRebuilt();

	TWeakObjectPtr<UCoreKitNavigationSubsystem> Navigation;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> EnhancedInput;
	FKey DisplayedKey;
	FCoreKitKeyPresentation Presentation;
};
