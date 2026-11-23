#pragma once

#include "CoreKitNavigation/CoreKitPromptWidget.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUIConfirmPrompt.generated.h"

class APlayerController;
class UCoreKitButton;
class UImage;
class UTextBlock;
class UWidget;

/** Destructive / confirmation prompts of the spec (Docs/UI/Spec/04-Overlays.md § Confirmación destructiva). */
UENUM(BlueprintType)
enum class ECoreKitBasicUIPromptPreset : uint8
{
	/** Pause > RESTART RUN: "Restart run?" with the run time. */
	RestartRun,
	/** Pause > QUIT TO MENU: "Quit to menu?" with time, level and cards. */
	QuitToMenu,
	/** Main Menu > QUIT: "Quit to desktop?" */
	QuitToDesktop,
	/** Settings > Reset (T): "Reset to defaults?" */
	ResetDefaults,
	/** Leaving Settings with unapplied changes: "Discard changes?" */
	DiscardChanges
};

/**
 * Styled confirmation modal (design system): 680 px dialog, chamfer 22, 12 px magenta tape on top with "CONFIRM",
 * title, body, CANCEL (ESC) + action button (ENTER, danger red by default), over a rgba(10,10,12,.72) veil.
 * Return cancels; Enter / A confirms the focused button (the action button has the initial focus, as in the mockup).
 * Use CoreKitBasicUIPrompt::Show / ShowPreset; it is pushed on the "Modal" layer of CoreKitNavigation.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIConfirmPrompt : public UCoreKitPromptWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIConfirmPrompt(const FObjectInitializer& ObjectInitializer);

	/** Danger red action (destructive) or magenta (system confirm). */
	void SetDanger(bool bInDanger) { bDanger = bInDanger; }

	/** Called once with true (action) or false (cancel / Return). */
	TFunction<void(bool /*bConfirmed*/)> OnConfirmed;

	static constexpr int32 CancelIndex = 0;
	static constexpr int32 ConfirmIndex = 1;

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	void NativeOnPromptSetup() override;

private:
	UFUNCTION()
	void HandleDecided(int32 Option);
	UFUNCTION()
	void HandleButtonSelected(UCoreKitButton* Button);
	UFUNCTION()
	void HandleButtonDeselected(UCoreKitButton* Button);
	void StyleButton(UCoreKitButton* Button, bool bSelected);

	UPROPERTY(Transient) TObjectPtr<UImage> Veil;
	UPROPERTY(Transient) TObjectPtr<UWidget> Dialog;

	FCoreKitBasicUITween Appear;
	bool bDanger = true;
	bool bLowPass = false;
};

namespace CoreKitBasicUIPrompt
{
	/** Title, body and action label of a preset, with real run data where the spec asks for it. */
	COREKITBASICUI_API void PresetTexts(ECoreKitBasicUIPromptPreset Preset, float RunSeconds, int32 Level, int32 Cards, FText& OutTitle, FText& OutBody, FText& OutConfirm);

	/** Pushes a styled prompt on the Modal layer. OnDecided(true) = action, (false) = cancel. */
	COREKITBASICUI_API UCoreKitBasicUIConfirmPrompt* Show(APlayerController* Controller, const FText& Title, const FText& Body, const FText& ConfirmLabel,
		TFunction<void(bool)> OnDecided, bool bDanger = true);

	/** Show with a preset's texts. */
	COREKITBASICUI_API UCoreKitBasicUIConfirmPrompt* ShowPreset(APlayerController* Controller, ECoreKitBasicUIPromptPreset Preset, TFunction<void(bool)> OnDecided,
		float RunSeconds = 0.f, int32 Level = 1, int32 Cards = 0);
}
