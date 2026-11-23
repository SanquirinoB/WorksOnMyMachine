#pragma once

#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "CoreKitFrontendMenu.generated.h"

class UButton;
class UCoreKitFrontendSubsystem;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

UCLASS(Blueprintable)
class COREKITFRONTEND_API UCoreKitFrontendMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CoreKit|Frontend")
	UCoreKitFrontendSubsystem* GetFrontend() const;

	/** Override when using a custom Designer layout. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Frontend")
	UWidget* GetInitialFocus() const;

	/** Configure a separate settings widget before attaching it to a project menu. */
	void ConfigureEmbeddedSettings(FSimpleDelegate InClose, const FText& InControlsHelp);

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	/** Read the subsystem's state, status and Continue availability here in a custom widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit|Frontend")
	void OnFrontendUpdated();

	struct FVideoOptions
	{
		FIntPoint Resolution = FIntPoint::ZeroValue;
		int32 WindowMode = 0;
		int32 Quality = -1;
		float FrameRate = 0.f;
		bool bVSync = false;

		bool operator==(const FVideoOptions& Other) const = default;
	};

	/** Custom native layouts share the draft and display preview lifecycle. */
	bool CanChangeDisplay() const;
	bool HasDraft() const;
	bool IsDisplayPreview() const { return PreviewDeadline > 0.0; }
	virtual void RefreshOptions();
	UFUNCTION() virtual void Back();
	UFUNCTION() void Apply();
	UFUNCTION() void Discard();
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusLabel;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyButton;
	UPROPERTY(Transient) TObjectPtr<UButton> DiscardButton;
	UPROPERTY(Transient) TObjectPtr<UButton> BackButton;

private:
	UButton* AddButton(UVerticalBox& Page, FName Name, const FText& Text, FName Callback);
	void BuildSettings(UVerticalBox& Page);
	UVerticalBox& AddOptionsSection(UVerticalBox& Page, const FText& Label, FName Callback);
	void ReadOptions();
	void ReadVideoOptions();
	void ResetDisplayPreview();
	void CommitOptions();

	void AdvanceVolume(int32 Index);
	void RefreshControls();
	void SetOptionsStatus(const FText& Message);
	void CancelCapture();
	void HandleAppActivation(bool bActive);

	UFUNCTION()
	void RefreshPage();
	UFUNCTION()
	void NewGame();
	UFUNCTION()
	void ContinueGame();
	UFUNCTION()
	void OpenSettings();

	UFUNCTION()
	void CycleWindowMode();
	UFUNCTION()
	void CycleResolution();
	UFUNCTION()
	void CycleQuality();
	UFUNCTION()
	void CycleFrameRate();
	UFUNCTION()
	void ToggleVSync();
	UFUNCTION()
	void CycleMaster();
	UFUNCTION()
	void CycleMusic();
	UFUNCTION()
	void CycleEffects();
	UFUNCTION()
	void SelectBinding();
	UFUNCTION()
	void CaptureBinding();
	UFUNCTION()
	void ResetBindings();
	UFUNCTION()
	void SelectVideo();
	UFUNCTION()
	void SelectAudio();
	UFUNCTION()
	void SelectControls();
	UFUNCTION()
	void KeyCaptured(bool bCaptured, FKey Key);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> EmbeddedSections;
	UPROPERTY(Transient)
	TObjectPtr<class UHorizontalBox> EmbeddedTabs;
	UPROPERTY(Transient)
	TObjectPtr<UButton> VideoTab;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NewButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> OptionButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> AudioButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ControlButtons;

protected:
	FVideoOptions DraftVideo;
	FVector3f DraftAudio = FVector3f(1.f);

private:
	FVideoOptions CommittedVideo;
	FVector3f CommittedAudio = FVector3f(1.f);
	double PreviewDeadline = 0.0;
	int32 BindingIndex = 0;
	bool bSettingsOpen = false;
	bool bCapturing = false;
	FSimpleDelegate EmbeddedClose;
	FText ControlsHelp;
};
