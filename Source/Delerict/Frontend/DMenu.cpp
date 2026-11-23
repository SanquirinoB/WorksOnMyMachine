#include "DMenu.h"
#include "DMenuStyle.h"
#include "DSettings.h"
#include "../LevelDesign/DRoomProgress.h"
#include "Components/NativeWidgetHost.h"
#include "Rendering/DrawElementTypes.h"
#include "Widgets/SLeafWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "DMenu"

using namespace DelerictUI;

namespace
{
class SDeathPulse final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SDeathPulse) {}
    SLATE_END_ARGS()
    void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }
    FVector2D ComputeDesiredSize(float) const override { return FVector2D(0, 32); }
    int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
    {
        TArray<FVector2f> Points = {{0, 14}, {45, 14}, {50, 24}, {56, 4}, {62, 30},
            {69, 14}, {static_cast<float>(Geometry.GetLocalSize().X), 14}};
        FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points),
            ESlateDrawEffect::None, Accent, true, 2.5f);
        return Layer;
    }
};

void SetPrimary(UButton& Button, bool bPrimary)
{
    auto* Row = CastChecked<UHorizontalBox>(Button.GetContent());
    auto* Rail = CastChecked<UBorder>(Row->GetChildAt(0));
    auto* Arrow = CastChecked<UTextBlock>(Row->GetChildAt(2));
    Rail->SetBrushColor(bPrimary ? Accent : Hex(0x64808A, .55f));
    Arrow->SetVisibility(bPrimary ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    Button.SetStyle(bPrimary ? ButtonStyle(Hex(0x332118, .95f), Hex(0xA26A42), 1.5f)
        : ButtonStyle(Hex(0x0B141B, .87f), FLinearColor::Transparent, 0.f));
}

void Fill(UOverlay& Overlay, UWidget& Child)
{
    UOverlaySlot* Placement = Overlay.AddChildToOverlay(&Child);
    Placement->SetHorizontalAlignment(HAlign_Fill);
    Placement->SetVerticalAlignment(VAlign_Fill);
}
}

TSharedRef<SWidget> UDMenu::RebuildWidget()
{
    if (WidgetTree->RootWidget)
    {
        return Super::RebuildWidget();
    }
    Overlay = WidgetTree->ConstructWidget<UOverlay>();
    Overlay->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    WidgetTree->RootWidget = Overlay;
    Actions = WidgetTree->ConstructWidget<UVerticalBox>();
    MenuPanel = Actions;
    const bool bOverlay = Screen != EScreen::Title;
    if (bOverlay)
    {
        auto* Shade = WidgetTree->ConstructWidget<UBorder>();
        Shade->SetBrushColor(FLinearColor(.005f, .009f, .013f, Screen == EScreen::Death ? .67f : .55f));
        Fill(*Overlay, *Shade);
        auto* Frame = Surface(*WidgetTree, Panel, FMargin(42, 30, 42, 42), 1.f);
        Frame->SetContent(Actions);
        MenuPanel = Frame;
    }
    else
    {
        auto* Art = WidgetTree->ConstructWidget<UImage>();
        const TSoftObjectPtr<UTexture2D> Backdrop(FSoftObjectPath(TEXT("/Game/Delerict/Frontend/T_MainMenuBackdrop.T_MainMenuBackdrop")));
        Art->SetBrushFromTexture(Backdrop.LoadSynchronous(), true);
        auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
        Scale->SetStretch(EStretch::ScaleToFill);
        Scale->SetContent(Art);
        Scale->SetVisibility(ESlateVisibility::HitTestInvisible);
        Fill(*Overlay, *Scale);
    }
    UOverlaySlot* Placement = Overlay->AddChildToOverlay(MenuPanel);
    Placement->SetHorizontalAlignment(HAlign_Left);
    Placement->SetVerticalAlignment(VAlign_Center);
    Placement->SetPadding(FMargin(112.f, 0.f, 24.f, bOverlay ? 0.f : 42.f));
    if (Screen == EScreen::Death)
    {
        BuildDeathCard();
        return Super::RebuildWidget();
    }
    const bool bPause = Screen == EScreen::Pause;
    auto* Heading = Label(*WidgetTree, bPause ? LOCTEXT("Paused", "Paused") : LOCTEXT("Title", "DERELICT"),
        bPause ? 57 : 143, Family::Display, bPause ? 700 : 800, Ink, bPause ? .015f : .04f);
    Heading->SetShadowColorAndOpacity(Hex(0x000000, .35f));
    Heading->SetShadowOffset(FVector2D(0, 3));
    Actions->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0, 0, 0, bPause ? 20 : -23));
    if (bPause)
    {
        PrimaryButton = AddAction(*Actions, TEXT("Resume"), LOCTEXT("Resume", "Resume"), GET_FUNCTION_NAME_CHECKED(ThisClass, Resume));
        UButton* Checkpoint = AddAction(*Actions, TEXT("RestartCheckpoint"), LOCTEXT("RestartCheckpoint", "Return to Last Checkpoint"),
            GET_FUNCTION_NAME_CHECKED(ThisClass, RestartCheckpoint));
        Checkpoint->SetIsEnabled(bHasCheckpoint);
        Checkpoint->SetRenderOpacity(bHasCheckpoint ? 1.f : .5f);
    }
    else
    {
        ContinueAction = AddAction(*Actions, TEXT("Continue"), LOCTEXT("Continue", "Continue"), GET_FUNCTION_NAME_CHECKED(ThisClass, Continue));
        PrimaryButton = AddAction(*Actions, TEXT("NewGame"), LOCTEXT("NewGame", "New Game"), GET_FUNCTION_NAME_CHECKED(ThisClass, StartNewGame));
    }
    SetPrimary(*PrimaryButton, true);
    OptionsButton = AddAction(*Actions, TEXT("Settings"), LOCTEXT("Settings", "Settings"), GET_FUNCTION_NAME_CHECKED(ThisClass, Settings));
    if (bPause)
    {
        AddAction(*Actions, TEXT("ReturnToMainMenu"), LOCTEXT("Return", "Return to Main Menu"), GET_FUNCTION_NAME_CHECKED(ThisClass, ReturnToTitle));
    }
    else
    {
        AddAction(*Actions, TEXT("ExitToDesktop"), LOCTEXT("Exit", "Exit to Desktop"), GET_FUNCTION_NAME_CHECKED(ThisClass, Exit));
        Feedback = Label(*WidgetTree, FText::GetEmpty(), 18);
        Feedback->SetAutoWrapText(true);
        Actions->AddChildToVerticalBox(Feedback);
    }
    BuildPrompt();
    return Super::RebuildWidget();
}

void UDMenu::BuildDeathCard()
{
    auto* Pulse = WidgetTree->ConstructWidget<UNativeWidgetHost>();
    Pulse->SetContent(SNew(SDeathPulse));
    Actions->AddChildToVerticalBox(Pulse)->SetPadding(FMargin(0, 8, 0, 14));
    Actions->AddChildToVerticalBox(Label(*WidgetTree, LOCTEXT("Died", "You Died"), 65, Family::Display, 700));
    Feedback = Label(*WidgetTree, bHasCheckpoint ? LOCTEXT("RetryCheckpoint", "Return to your last checkpoint.")
        : LOCTEXT("RetryStart", "Return to the level start."), 23);
    Actions->AddChildToVerticalBox(Feedback)->SetPadding(FMargin(0, 8, 0, 34));
    PrimaryButton = AddAction(*Actions, TEXT("Retry"), LOCTEXT("Retry", "Retry"), GET_FUNCTION_NAME_CHECKED(ThisClass, Retry));
    SetPrimary(*PrimaryButton, true);
}

UButton* UDMenu::AddAction(UVerticalBox& Column, FName Name, const FText& Text, FName Handler)
{
    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* Rail = Surface(*WidgetTree, FLinearColor::White);
    auto* RailWidth = WidgetTree->ConstructWidget<USizeBox>();
    RailWidth->SetWidthOverride(4.f);
    Rail->SetContent(RailWidth);
    Row->AddChildToHorizontalBox(Rail);
    auto* Caption = Label(*WidgetTree, Text, 19, Family::Body, 600);
    auto* CaptionSlot = Row->AddChildToHorizontalBox(Caption);
    CaptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CaptionSlot->SetVerticalAlignment(VAlign_Center);
    CaptionSlot->SetPadding(FMargin(22, 0));
    auto* Arrow = Label(*WidgetTree, FText::FromString(TEXT("›")), 33, Family::Body, 600, Accent);
    auto* ArrowSlot = Row->AddChildToHorizontalBox(Arrow);
    ArrowSlot->SetVerticalAlignment(VAlign_Center);
    ArrowSlot->SetPadding(FMargin(0, 0, 22, 0));
    auto* Action = Button(*WidgetTree, *this, Name, Handler, Row);
    SetPrimary(*Action, false);
    auto* Placement = Column.AddChildToVerticalBox(Box(*WidgetTree, Action, Screen == EScreen::Title ? 505.f : 450.f, 71.f));
    Placement->SetHorizontalAlignment(HAlign_Left);
    Placement->SetPadding(FMargin(0, 0, 0, 8));
    return Action;
}

void UDMenu::BuildPrompt()
{
    Prompt = WidgetTree->ConstructWidget<UBorder>();
    Prompt->SetBrushColor(FLinearColor(.003f, .005f, .008f, .82f));
    Prompt->SetHorizontalAlignment(HAlign_Center);
    Prompt->SetVerticalAlignment(VAlign_Center);
    Fill(*Overlay, *Prompt);
    auto* Card = WidgetTree->ConstructWidget<UVerticalBox>();
    Prompt->SetContent(Box(*WidgetTree, Card, 670, 0));
    Card->AddChildToVerticalBox(Box(*WidgetTree, Surface(*WidgetTree, Accent), 0, 2));
    auto* Frame = Surface(*WidgetTree, Panel, FMargin(36), 1.5f);
    Card->AddChildToVerticalBox(Frame);
    auto* Body = WidgetTree->ConstructWidget<UVerticalBox>();
    Frame->SetContent(Body);
    PromptQuestion = Label(*WidgetTree, FText::GetEmpty(), 39, Family::Display, 700);
    PromptDetail = Label(*WidgetTree, FText::GetEmpty(), 20, Family::Body, 500, Muted);
    PromptDetail->SetAutoWrapText(true);
    Body->AddChildToVerticalBox(PromptQuestion);
    Body->AddChildToVerticalBox(PromptDetail)->SetPadding(FMargin(0, 8, 0, 28));
    auto* Choices = WidgetTree->ConstructWidget<UHorizontalBox>();
    Body->AddChildToVerticalBox(Choices)->SetHorizontalAlignment(HAlign_Right);
    auto AddChoice = [&](FName Name, UTextBlock* Caption, FName Handler)
    {
        auto* Choice = Button(*WidgetTree, *this, Name, Handler, Caption);
        auto* Slot = CastChecked<UButtonSlot>(Caption->Slot);
        Slot->SetVerticalAlignment(VAlign_Center);
        Slot->SetPadding(FMargin(18, 4));
        Choices->AddChildToHorizontalBox(Box(*WidgetTree, Choice, 0, 57))->SetPadding(FMargin(10, 0, 0, 0));
        return Choice;
    };
    CancelButton = AddChoice(TEXT("Cancel"), Label(*WidgetTree, LOCTEXT("Cancel", "Cancel"), 19),
        GET_FUNCTION_NAME_CHECKED(ThisClass, CancelPrompt));
    ConfirmText = Label(*WidgetTree, FText(), 19, Family::Body, 600, Hex(0x23170E));
    auto* Confirm = AddChoice(TEXT("Confirm"), ConfirmText, GET_FUNCTION_NAME_CHECKED(ThisClass, ConfirmPrompt));
    Confirm->SetStyle(AccentButtonStyle());
    Prompt->SetVisibility(ESlateVisibility::Collapsed);
}

void UDMenu::NativeConstruct()
{
    Super::NativeConstruct();
    if (Screen == EScreen::Title && GetFrontend())
    {
        GetFrontend()->OnChanged.AddUniqueDynamic(this, &ThisClass::RefreshTitle);
        RefreshTitle();
    }
}

void UDMenu::NativeDestruct()
{
    ReleaseFocus(ChildFocus);
    ReleaseFocus(PauseFocus);
    if (auto* Frontend = GetFrontend())
    {
        Frontend->OnChanged.RemoveDynamic(this, &ThisClass::RefreshTitle);
    }
    Super::NativeDestruct();
}

UWidget* UDMenu::GetInitialFocus_Implementation() const
{
    return ContinueAction && ContinueAction->GetIsEnabled() ? ContinueAction.Get() : PrimaryButton.Get();
}

void UDMenu::RefreshTitle()
{
    const auto* Frontend = GetFrontend();
    const bool bCanContinue = Frontend->CanContinue();
    const bool bRestoreFocus = bCanContinue && !ContinueAction->GetIsEnabled() &&
        PrimaryButton->HasUserFocus(GetOwningPlayer()) && !ChildFocus.IsValid();
    ContinueAction->SetIsEnabled(bCanContinue);
    SetPrimary(*ContinueAction, bCanContinue);
    SetPrimary(*PrimaryButton, !bCanContinue);
    const bool bIdle = Frontend->GetState() == ECoreKitFrontendState::MainMenu;
    PrimaryButton->SetIsEnabled(bIdle);
    OptionsButton->SetIsEnabled(bIdle);
    Feedback->SetText(Frontend->GetStatus());
    Feedback->SetVisibility(Frontend->GetStatus().IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    if (bRestoreFocus)
    {
        ContinueAction->SetUserFocus(GetOwningPlayer());
    }
}

bool UDMenu::OpenPause(APlayerController& Player)
{
    const auto* Frontend = Player.GetGameInstance()->GetSubsystem<UCoreKitFrontendSubsystem>();
    return Frontend && Frontend->GetState() == ECoreKitFrontendState::Gameplay &&
        OpenOverlay(Player, EScreen::Pause, Frontend->CanRestartFromCheckpoint());
}

bool UDMenu::OpenDeath(APlayerController& Player, bool bCheckpoint)
{
    return OpenOverlay(Player, EScreen::Death, bCheckpoint);
}

bool UDMenu::OpenOverlay(APlayerController& Player, EScreen InScreen, bool bCheckpoint)
{
    auto* Local = Player.GetLocalPlayer();
    if (!Local || UGameplayStatics::IsGamePaused(&Player))
    {
        return false;
    }
    auto* Menu = CreateWidget<UDMenu>(&Player);
    if (!Menu) { return false; }
    Menu->Screen = InScreen;
    Menu->bHasCheckpoint = bCheckpoint;
    Menu->bPreviousCursor = Player.ShouldShowMouseCursor();
    Menu->PreviousCursorShape = Player.CurrentMouseCursor;
    if (!UGameplayStatics::SetGamePaused(&Player, true))
    {
        return false;
    }
    if (!Menu->AddToPlayerScreen(90))
    {
        UGameplayStatics::SetGamePaused(&Player, false);
        return false;
    }
    UWidget* Focus = Menu->GetInitialFocus();
    Player.CurrentMouseCursor = EMouseCursor::Default;
    Player.SetShowMouseCursor(true);
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(&Player, Focus, EMouseLockMode::DoNotLock, true);
    Menu->PauseFocus = Local->GetSubsystem<UCoreKitNavigationSubsystem>()->PushMenuFocusScope(Focus, Menu);
    return true;
}

void UDMenu::CloseOverlay()
{
    APlayerController* Player = GetOwningPlayer();
    ReleaseFocus(PauseFocus);
    Player->SetShowMouseCursor(bPreviousCursor);
    Player->CurrentMouseCursor = PreviousCursorShape;
    UWidgetBlueprintLibrary::SetInputMode_GameAndUIEx(Player, nullptr, EMouseLockMode::LockInFullscreen, false, true);
    RemoveFromParent();
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void UDMenu::Settings()
{
    if (Options)
    {
        return;
    }
    Options = CreateWidget<UDSettings>(GetOwningPlayer());
    Options->ConfigureEmbeddedSettings(FSimpleDelegate::CreateUObject(this, &ThisClass::CloseSettings), FText());
    MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
    if (Screen == EScreen::Title)
    {
        SettingsShade = Surface(*WidgetTree, Hex(0x000000, .42f));
        Fill(*Overlay, *SettingsShade);
    }
    Fill(*Overlay, *Options);
    ChildFocus = GetOwningLocalPlayer()->GetSubsystem<UCoreKitNavigationSubsystem>()->PushMenuFocusScope(Options->GetInitialFocus(), Options);
}

void UDMenu::CloseSettings()
{
    MenuPanel->SetVisibility(ESlateVisibility::Visible);
    if (SettingsShade)
    {
        SettingsShade->RemoveFromParent();
        SettingsShade = nullptr;
    }
    ReleaseFocus(ChildFocus);
    Options->RemoveFromParent();
    Options = nullptr;
    OptionsButton->SetUserFocus(GetOwningPlayer());
}

void UDMenu::ReleaseFocus(FGuid& Scope)
{
    if (Scope.IsValid() && GetOwningLocalPlayer())
    {
        GetOwningLocalPlayer()->GetSubsystem<UCoreKitNavigationSubsystem>()->ReleaseMenuFocusScope(Scope);
    }
    Scope.Invalidate();
}

FReply UDMenu::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const FKey Key = Event.GetKey();
    if (!Options && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right))
    {
        if (Screen == EScreen::Death) { return FReply::Handled(); }
        if (Prompt->GetVisibility() == ESlateVisibility::Visible)
        {
            CancelPrompt();
            return FReply::Handled();
        }
        if (Screen == EScreen::Pause)
        {
            Resume();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UDMenu::OpenPrompt(EPrompt Action, const FText& Question, const FText& Detail)
{
    PendingAction = Action;
    ConfirmText->SetText(Action == EPrompt::Exit ? LOCTEXT("ExitGame", "Exit Game") :
        Action == EPrompt::Return ? LOCTEXT("Return", "Return to Main Menu") : LOCTEXT("NewGame", "New Game"));
    PromptQuestion->SetText(Question);
    PromptDetail->SetText(Detail);
    Prompt->SetVisibility(ESlateVisibility::Visible);
    ChildFocus = GetOwningLocalPlayer()->GetSubsystem<UCoreKitNavigationSubsystem>()->PushMenuFocusScope(CancelButton, this);
    Actions->SetIsEnabled(false);
}

void UDMenu::CancelPrompt()
{
    Actions->SetIsEnabled(true);
    ReleaseFocus(ChildFocus);
    Prompt->SetVisibility(ESlateVisibility::Collapsed);
}

void UDMenu::ConfirmPrompt()
{
    CancelPrompt();
    switch (PendingAction)
    {
    case EPrompt::NewGame:
        GetFrontend()->NewGame();
        break;
    case EPrompt::Return:
        if (UGameplayStatics::SetGamePaused(this, false))
        {
            CloseOverlay();
            GetFrontend()->ReturnToMainMenu();
        }
        break;
    case EPrompt::Exit:
        UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
        break;
    }
}

void UDMenu::Continue() { GetFrontend()->ContinueGame(); }
void UDMenu::StartNewGame()
{
    if (GetFrontend()->CanContinue())
    {
        OpenPrompt(EPrompt::NewGame, LOCTEXT("ConfirmNew", "Start a New Game?"),
            LOCTEXT("ConfirmNewDesc", "Your first save in the new game will replace the existing progress."));
    }
    else
    {
        GetFrontend()->NewGame();
    }
}
void UDMenu::Resume()
{
    if (UGameplayStatics::SetGamePaused(this, false))
    {
        CloseOverlay();
    }
}
void UDMenu::Retry()
{
    APlayerController* Player = GetOwningPlayer();
    auto* Progress = GetWorld()->GetSubsystem<UDRoomProgress>();
    if (!Player || !Progress || !Progress->Retry(*Player))
    {
        Feedback->SetText(LOCTEXT("RetryFailed", "Could not respawn. Try again."));
        return;
    }
    UGameplayStatics::SetGamePaused(this, false);
    CloseOverlay();
}

void UDMenu::RestartCheckpoint()
{
    auto* Frontend = GetFrontend();
    if (Frontend && Frontend->CanRestartFromCheckpoint() && Frontend->RestartFromCheckpoint())
    {
        UGameplayStatics::SetGamePaused(this, false);
        CloseOverlay();
    }
}

void UDMenu::ReturnToTitle()
{
    OpenPrompt(EPrompt::Return, LOCTEXT("ConfirmReturn", "Return to Main Menu?"),
        LOCTEXT("ConfirmReturnDesc", "Unsaved progress will be lost."));
}
void UDMenu::Exit()
{
    OpenPrompt(EPrompt::Exit, LOCTEXT("ConfirmExit", "Exit to Desktop?"),
        LOCTEXT("ConfirmExitDesc", "The game will close."));
}

#undef LOCTEXT_NAMESPACE
