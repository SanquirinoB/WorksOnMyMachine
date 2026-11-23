#pragma once

#include "Components/Button.h"
#include "CoreKitButton.generated.h"

class UCoreKitMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitButtonEvent, class UCoreKitButton*, Button);

/**
 * Button with a single "selected" state: hovering focuses it, and keyboard/gamepad focus shows the hovered style.
 * Ignores activation while its owning Core Kit menu is in its activation cooldown and plays the project's
 * navigation sounds.
 */
UCLASS(meta = (DisplayName = "Core Kit Button"))
class COREKITNAVIGATION_API UCoreKitButton : public UButton
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitButtonEvent OnSelected;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitButtonEvent OnDeselected;

	/** Click, Enter or gamepad accept that passed the menu cooldown. */
	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitButtonEvent OnActivated;

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	bool IsSelected() const { return bSelected; }

	/** Focus this button for its owning player. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void Select();

	/** Use instead of SetStyle: keeps the selected (focus) look in sync with the new style. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetBaseStyle(const FButtonStyle& InStyle);

	/** Hover focuses the button (single selection). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	bool bSelectOnHover = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	bool bPlaySounds = true;

	/** Must be called before the button is first displayed. */
	void SetFocusableBeforeConstruct(bool bFocusable) { InitIsFocusable(bFocusable); }

	/** Free value for menus that build lists of buttons (index, option id...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	int32 ButtonIndex = INDEX_NONE;

	void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	TSharedRef<SWidget> RebuildWidget() override;

private:
	FReply HandleClicked();
	void HandleHovered();
	void HandleUnhovered();
	void HandleFocusReceived();
	void HandleFocusLost();
	void SetSelected(bool bInSelected);
	void ApplySelectedStyle();
	UCoreKitMenuWidget* FindOwningMenu() const;

	FButtonStyle BaseStyle;
	bool bHasBaseStyle = false;
	bool bSelected = false;
};
