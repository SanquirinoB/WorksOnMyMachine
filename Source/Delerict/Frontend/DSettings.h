#pragma once

#include "CoreKitFrontend/CoreKitFrontendMenu.h"
#include "DSettings.generated.h"

class UBorder;
class UComboBoxString;
class UHorizontalBox;
class UProgressBar;
class USlider;

UCLASS()
class UDSettings : public UCoreKitFrontendMenu
{
    GENERATED_BODY()

public:
    UWidget* GetInitialFocus_Implementation() const override;
protected:
    TSharedRef<SWidget> RebuildWidget() override;
    void RefreshOptions() override;
    void Back() override;
private:
    UHorizontalBox* AddRow(UVerticalBox& Page, const FText& Name);
    UButton* FooterButton(FName Name, const FText& Text, FName Action, bool bAccent);
    void AddVolume(UVerticalBox& Page, FName Name, const FText& Title, int32 Index);
    void SetVolume(int32 Index, float Value);
    void SelectSection(int32 Index);
    void RefreshResolutions();
    void ShowDialog(bool bDisplay);
    void HideDialog();
    UFUNCTION() void VideoTabClicked();
    UFUNCTION() void AudioTabClicked();
    UFUNCTION() void ControlsTabClicked();
    UFUNCTION() void SelectionChanged(FString Value, ESelectInfo::Type Type);
    UFUNCTION() UWidget* OptionText(FString Value);
    UFUNCTION() void SetMaster(float Value);
    UFUNCTION() void SetMusic(float Value);
    UFUNCTION() void CancelDialog();
    UFUNCTION() void AcceptDialog();

    UPROPERTY(Transient) TObjectPtr<UVerticalBox> PanelBody;
    UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Sections;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> Tabs;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Underlines;
    UPROPERTY(Transient) TArray<TObjectPtr<UComboBoxString>> Fields;
    UPROPERTY(Transient) TArray<TObjectPtr<USlider>> Sliders;
    UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> Tracks;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Percentages;
    UPROPERTY(Transient) TObjectPtr<UButton> FooterApply;
    UPROPERTY(Transient) TObjectPtr<UButton> FooterDiscard;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FooterStatus;
    UPROPERTY(Transient) TObjectPtr<UBorder> Dialog;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DialogTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DialogMessage;
    UPROPERTY(Transient) TObjectPtr<UButton> DialogCancel;
    UPROPERTY(Transient) TObjectPtr<UButton> DialogAccept;
    TArray<FIntPoint> Resolutions;
    TArray<float> FrameRates;
    int32 ResolutionMode = INDEX_NONE;
    bool bRefreshing = false;
    bool bDisplayDialog = false;
};
