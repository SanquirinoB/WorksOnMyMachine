#pragma once

#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIPauseMenu.generated.h"

class UBackgroundBlur;
class UCanvasPanel;
class UCoreKitBasicUIActionButton;
class UCoreKitBasicUIKeycap;
class UCoreKitBasicUITabularNumber;
class UImage;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * Pause overlay (Docs/UI/Spec/04-Overlays.md § Pausa, artboard PauseMenu), pushed on the "Menu" layer by
 * UCoreKitBasicUIScreenRouter while the run is paused manually. Blur/dim of the world, "SIMULATION HALTED / PAUSED", five entries
 * (RESUME, HOW TO PLAY, SETTINGS, RESTART RUN, QUIT TO MENU), run status tiles, the build list and the footer.
 * Return resumes. HOW TO PLAY / SETTINGS open the front-end screens on top; RESTART / QUIT ask a styled confirmation.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIPauseMenu : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIPauseMenu(const FObjectInitializer& ObjectInitializer);

	/** Exit animation (140 ms EXIT, all together) then CloseMenu. Called by the router when the run resumes. */
	void BeginExit();
	bool IsClosing() const { return bClosing; }

	/** RESUME: asks the run to unpause (the router closes the menu when the Run ViewModel reports it). */
	void Resume();

protected:
	void BuildTree() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void TickMotion(float DeltaSeconds) override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;

private:
	void HandleEntry(UCoreKitBasicUIActionButton* Button);
	void RefreshStatus();
	void RefreshBuild();
	void RefreshFooterKeys();
	UWidget* MakeTile(const FText& Label, TObjectPtr<UCoreKitBasicUITabularNumber>& OutValue, TObjectPtr<UTextBlock>* OutSub, FLinearColor ValueColor, bool bBar);

	UPROPERTY(Transient) TObjectPtr<UBackgroundBlur> Blur;
	UPROPERTY(Transient) TObjectPtr<UWidget> Veil;
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> StatusGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> FooterGroup;
	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIActionButton>> Entries;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> TimeValue;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> StageValue;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> LevelValue;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUITabularNumber> KillsValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StageSub;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelSub;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KillsSub;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TimeSub;
	UPROPERTY(Transient) TObjectPtr<UImage> TimeBarFill;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BuildHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> BuildList;
	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIKeycap>> FooterKeys;

	FCoreKitBasicUITween Enter;
	FCoreKitBasicUITween Leave;
	float Clock = 0.f;
	float RefreshTimer = 0.f;
	int32 BuildSerial = INDEX_NONE;
	bool bClosing = false;
	bool bLowPass = false;
	bool bPadKeys = false;
	bool bKeysSet = false;
};
