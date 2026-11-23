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

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	/** Read the subsystem's state, status and Continue availability here in a custom widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit|Frontend")
	void OnFrontendUpdated();

private:
	struct FVideoOptions
	{
		FIntPoint Resolution = FIntPoint::ZeroValue;
		int32 WindowMode = 0;
		int32 Quality = -1;
		float FrameRate = 0.f;
		bool bVSync = false;

		bool operator==(const FVideoOptions& Other) const = default;
	};

	UButton* AddButton(UVerticalBox& Page, FName Name, const FText& Text, FName Callback);
	void BuildSettings(UVerticalBox& Page);
	void ReadOptions();
	void RefreshOptions();
	void ResetDisplayPreview();
	void CommitOptions();
	bool CanChangeDisplay() const;
	bool HasDraft() const;
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
	void Back();
	UFUNCTION()
	void Apply();
	UFUNCTION()
	void Discard();
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
	void KeyCaptured(bool bCaptured, FKey Key);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusLabel;
	UPROPERTY(Transient)
	TObjectPtr<UButton> NewButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ApplyButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> DiscardButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> OptionButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> AudioButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ControlButtons;
	FVideoOptions CommittedVideo;
	FVideoOptions DraftVideo;
	FVector3f CommittedAudio = FVector3f(1.f);
	FVector3f DraftAudio = FVector3f(1.f);
	double PreviewDeadline = 0.0;
	int32 BindingIndex = 0;
	bool bSettingsOpen = false;
	bool bCapturing = false;
};
