#pragma once

#include "CoreKitFrontend/CoreKitFrontendMenu.h"
#include "GenericPlatform/ICursor.h"
#include "DMenu.generated.h"

class UBorder;
class UButton;
class UOverlay;
class UTextBlock;
class UVerticalBox;

UCLASS()
class UDMenu : public UCoreKitFrontendMenu
{
    GENERATED_BODY()

public:
    static bool OpenPause(APlayerController& Player);
    static bool OpenDeath(APlayerController& Player, bool bHasCheckpoint);
    UWidget* GetInitialFocus_Implementation() const override;

protected:
    TSharedRef<SWidget> RebuildWidget() override;
    void NativeConstruct() override;
    void NativeDestruct() override;
    FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    enum class EScreen { Title, Pause, Death };
    static bool OpenOverlay(APlayerController& Player, EScreen Screen, bool bHasCheckpoint);
    void BuildDeathCard();
    enum class EPrompt { NewGame, Return, Exit };
    void OpenPrompt(EPrompt Action, const FText& Question, const FText& Detail);
    void CloseSettings();
    void ReleaseFocus(FGuid& Scope);
    void CloseOverlay();
    UButton* AddAction(UVerticalBox& Column, FName Name, const FText& Label, FName Handler);
    void BuildPrompt();

    UFUNCTION() void RefreshTitle();
    UFUNCTION() void Continue();
    UFUNCTION() void StartNewGame();
    UFUNCTION() void Settings();
    UFUNCTION() void Resume();
    UFUNCTION() void RestartCheckpoint();
    UFUNCTION() void Retry();
    UFUNCTION() void ReturnToTitle();
    UFUNCTION() void Exit();
    UFUNCTION() void CancelPrompt();
    UFUNCTION() void ConfirmPrompt();

    UPROPERTY(Transient) TObjectPtr<UOverlay> Overlay;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Actions;
    UPROPERTY(Transient) TObjectPtr<UButton> ContinueAction;
    UPROPERTY(Transient) TObjectPtr<UButton> PrimaryButton;
    UPROPERTY(Transient) TObjectPtr<UButton> OptionsButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
    UPROPERTY(Transient) TObjectPtr<UBorder> Prompt;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptQuestion;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptDetail;
    UPROPERTY(Transient) TObjectPtr<UButton> CancelButton;
    UPROPERTY(Transient) TObjectPtr<UCoreKitFrontendMenu> Options;
    UPROPERTY(Transient) TObjectPtr<UWidget> MenuPanel;
    UPROPERTY(Transient) TObjectPtr<UBorder> SettingsShade;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmText;
    FGuid PauseFocus;
    FGuid ChildFocus;
    EPrompt PendingAction = EPrompt::Exit;
    EScreen Screen = EScreen::Title;
    bool bHasCheckpoint = false;
    bool bPreviousCursor = false;
    EMouseCursor::Type PreviousCursorShape = EMouseCursor::Default;
};
