#include "DSettings.h"
#include "DMenuStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "DSettings"
using namespace DelerictUI;

namespace
{
void AddLine(UWidgetTree& Tree, UVerticalBox& Column, FLinearColor Color, float Height)
{
    Column.AddChildToVerticalBox(Box(Tree, Surface(Tree, Color), 0, Height));
}

FString ResolutionName(FIntPoint Size)
{
    return FString::Printf(TEXT("%d × %d"), Size.X, Size.Y);
}

void ButtonCaption(UButton* Button, const FText& Text)
{
    CastChecked<UTextBlock>(Button->GetContent())->SetText(Text);
}
}

TSharedRef<SWidget> UDSettings::RebuildWidget()
{
    if (WidgetTree->RootWidget) { return Super::RebuildWidget(); }
    UWidgetTree& Tree = *WidgetTree;
    auto* Root = Tree.ConstructWidget<UOverlay>();
    Tree.RootWidget = Root;

    auto* Card = Tree.ConstructWidget<UVerticalBox>();
    AddLine(Tree, *Card, Accent, 2.f);
    auto* Frame = Surface(Tree, Panel, FMargin(43, 30, 43, 42), 1.5f);
    Card->AddChildToVerticalBox(Frame);
    PanelBody = Tree.ConstructWidget<UVerticalBox>();
    Frame->SetContent(PanelBody);
    auto* Fit = Tree.ConstructWidget<UScaleBox>();
    Fit->SetStretch(EStretch::ScaleToFit);
    Fit->SetStretchDirection(EStretchDirection::DownOnly);
    auto* FitSlot = CastChecked<UScaleBoxSlot>(Fit->AddChild(Box(Tree, Card, 850, 0)));
    FitSlot->SetHorizontalAlignment(HAlign_Left);
    FitSlot->SetVerticalAlignment(VAlign_Center);
    auto* At = Root->AddChildToOverlay(Fit);
    At->SetHorizontalAlignment(HAlign_Fill);
    At->SetVerticalAlignment(VAlign_Fill);
    At->SetPadding(FMargin(92, 36, 52, 36));

    PanelBody->AddChildToVerticalBox(Label(Tree, LOCTEXT("Heading", "Settings"), 57, Family::Display, 700));
    auto* TabBar = Tree.ConstructWidget<UHorizontalBox>();
    PanelBody->AddChildToVerticalBox(TabBar)->SetPadding(FMargin(0, 8, 0, 0));
    const FName Names[] = { TEXT("SelectVideo"), TEXT("SelectAudio"), TEXT("SelectControls") };
    const FName Callbacks[] = { GET_FUNCTION_NAME_CHECKED(ThisClass, VideoTabClicked),
        GET_FUNCTION_NAME_CHECKED(ThisClass, AudioTabClicked), GET_FUNCTION_NAME_CHECKED(ThisClass, ControlsTabClicked) };
    const FText Captions[] = { LOCTEXT("Video", "Video"), LOCTEXT("Audio", "Audio"), LOCTEXT("Controls", "Controls") };
    Sections = Tree.ConstructWidget<UWidgetSwitcher>();
    for (int32 Index = 0; Index < 3; ++Index)
    {
        auto* Column = Tree.ConstructWidget<UVerticalBox>();
        auto* Text = Label(Tree, Captions[Index], 18, Family::Body, 600);
        auto* Tab = Button(Tree, *this, Names[Index], Callbacks[Index], Text);
        auto* TabSlot = CastChecked<UButtonSlot>(Text->Slot);
        TabSlot->SetPadding(FMargin(12, 6));
        TabSlot->SetVerticalAlignment(VAlign_Center);
        Tabs.Add(Tab);
        Column->AddChildToVerticalBox(Box(Tree, Tab, 0, 59));
        UBorder* Line = Surface(Tree, FLinearColor::White);
        Underlines.Add(Line);
        Column->AddChildToVerticalBox(Box(Tree, Line, 0, 3));
        TabBar->AddChildToHorizontalBox(Column)->SetPadding(FMargin(0, 0, 24, 0));
        Sections->AddChild(Tree.ConstructWidget<UVerticalBox>());
    }
    AddLine(Tree, *PanelBody, Edge, 1.5f);
    PanelBody->AddChildToVerticalBox(Box(Tree, Sections, 0, 410));

    auto* Video = CastChecked<UVerticalBox>(Sections->GetChildAt(0));
    const FName FieldNames[] = { TEXT("WindowMode"), TEXT("Resolution"), TEXT("Quality"), TEXT("FrameRate"), TEXT("VSync") };
    const FText Labels[] = { LOCTEXT("Mode", "Window Mode"), LOCTEXT("Resolution", "Resolution"),
        LOCTEXT("Quality", "Graphics Quality"), LOCTEXT("FrameRate", "Frame Rate Limit"), LOCTEXT("VSync", "VSync") };
    for (int32 Index = 0; Index < 5; ++Index)
    {
        auto* FieldWidget = Tree.ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), FieldNames[Index]);
        auto Style = FieldWidget->GetWidgetStyle();
        Style.ComboButtonStyle.ButtonStyle = ButtonStyle();
        FieldWidget->SetHasDownArrow(false);
        Style.ComboButtonStyle.ButtonStyle.NormalForeground = Ink;
        Style.ComboButtonStyle.ButtonStyle.HoveredForeground = Ink;
        Style.ComboButtonStyle.ButtonStyle.PressedForeground = Ink;
        Style.ComboButtonStyle.MenuBorderBrush = FSlateRoundedBoxBrush(Field, 0.f, Edge, 1.5f);
        Style.ComboButtonStyle.MenuBorderPadding = FMargin(2);
        FieldWidget->SetWidgetStyle(Style);
        auto Items = FieldWidget->GetItemStyle();
        Items.EvenRowBackgroundBrush = Items.OddRowBackgroundBrush = FSlateColorBrush(Field);
        Items.ActiveBrush = Items.InactiveBrush = FSlateColorBrush(Hex(0x503321));
        Items.EvenRowBackgroundHoveredBrush = Items.OddRowBackgroundHoveredBrush =
            Items.ActiveHoveredBrush = Items.InactiveHoveredBrush = FSlateColorBrush(Hex(0x344953));
        FieldWidget->SetItemStyle(Items);
        FieldWidget->SetContentPadding(FMargin(14, 3, 9, 3));
        FieldWidget->SetMaxListHeight(275.f);
        FieldWidget->OnGenerateWidgetEvent.BindDynamic(this, &ThisClass::OptionText);
        FieldWidget->OnSelectionChanged.AddDynamic(this, &ThisClass::SelectionChanged);
        Fields.Add(FieldWidget);
        auto* FieldOverlay = Tree.ConstructWidget<UOverlay>();
        auto* ControlSlot = FieldOverlay->AddChildToOverlay(FieldWidget);
        ControlSlot->SetHorizontalAlignment(HAlign_Fill);
        ControlSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Chevron = Label(Tree, FText::FromString(TEXT("›")), 21, Family::Body, 600, Ink);
        Chevron->SetRenderTransformAngle(90.f);
        auto* ChevronSlot = FieldOverlay->AddChildToOverlay(Chevron);
        ChevronSlot->SetHorizontalAlignment(HAlign_Right);
        ChevronSlot->SetVerticalAlignment(VAlign_Center);
        ChevronSlot->SetPadding(FMargin(0, 0, 12, 0));
        AddRow(*Video, Labels[Index])->AddChildToHorizontalBox(Box(Tree, FieldOverlay, 275, 56))->SetVerticalAlignment(VAlign_Center);
    }
    for (const TCHAR* Mode : { TEXT("Fullscreen"), TEXT("Borderless"), TEXT("Windowed") }) { Fields[0]->AddOption(Mode); }
    for (const TCHAR* Quality : { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") }) { Fields[2]->AddOption(Quality); }
    Fields[4]->AddOption(TEXT("Off"));
    Fields[4]->AddOption(TEXT("On"));

    auto* Audio = CastChecked<UVerticalBox>(Sections->GetChildAt(1));
    AddVolume(*Audio, TEXT("MasterVolume"), LOCTEXT("Master", "Master Volume"), 0);
    AddVolume(*Audio, TEXT("MusicVolume"), LOCTEXT("Music", "Music Volume"), 1);

    auto* Controls = CastChecked<UVerticalBox>(Sections->GetChildAt(2));
    auto AddBinding = [&](const FText& Action, const TArray<FText>& Bindings)
    {
        auto* Row = AddRow(*Controls, Action);
        for (const FText& Binding : Bindings)
        {
            auto* Cap = Surface(Tree, Field, FMargin(10, 4), 1.f);
            Cap->SetContent(Label(Tree, Binding, 14, Family::Body, 600));
            auto* KeySlot = Row->AddChildToHorizontalBox(Cap);
            KeySlot->SetVerticalAlignment(VAlign_Center);
            KeySlot->SetPadding(FMargin(7, 0, 0, 0));
        }
    };
    AddBinding(LOCTEXT("Move", "Move"), { LOCTEXT("WASD", "WASD"), LOCTEXT("Arrows", "Arrow Keys"), LOCTEXT("Stick", "Left Stick") });
    AddBinding(LOCTEXT("ChargeJump", "Charge Jump"), { LOCTEXT("Space", "Space"), LOCTEXT("Face", "Gamepad Bottom Button") });
    AddBinding(LOCTEXT("Pause", "Pause"), { LOCTEXT("Escape", "Esc"), LOCTEXT("MenuKey", "Gamepad Menu Button") });

    FooterStatus = Label(Tree, FText::GetEmpty(), 17, Family::Body, 500, Accent);
    FooterStatus->SetAutoWrapText(true);
    PanelBody->AddChildToVerticalBox(Box(Tree, FooterStatus, 0, 62))->SetPadding(FMargin(0, 5, 0, 0));
    auto* Footer = Tree.ConstructWidget<UHorizontalBox>();
    PanelBody->AddChildToVerticalBox(Box(Tree, Footer, 0, 60));
    BackButton = FooterButton(TEXT("Back"), LOCTEXT("Back", "‹  Back"), GET_FUNCTION_NAME_CHECKED(ThisClass, Back), false);
    Footer->AddChildToHorizontalBox(BackButton);
    Footer->AddChildToHorizontalBox(Tree.ConstructWidget<USizeBox>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    FooterDiscard = FooterButton(TEXT("Discard"), LOCTEXT("Discard", "Discard"), GET_FUNCTION_NAME_CHECKED(ThisClass, Discard), false);
    FooterApply = FooterButton(TEXT("Apply"), LOCTEXT("Apply", "Apply"), GET_FUNCTION_NAME_CHECKED(ThisClass, Apply), true);
    Footer->AddChildToHorizontalBox(FooterDiscard)->SetPadding(FMargin(0, 0, 10, 0));
    Footer->AddChildToHorizontalBox(FooterApply);

    Dialog = Surface(Tree, Hex(0x02070B, .80f));
    Dialog->SetHorizontalAlignment(HAlign_Center);
    Dialog->SetVerticalAlignment(VAlign_Center);
    auto* ModalSlot = Root->AddChildToOverlay(Dialog);
    ModalSlot->SetHorizontalAlignment(HAlign_Fill);
    ModalSlot->SetVerticalAlignment(VAlign_Fill);
    auto* Modal = Tree.ConstructWidget<UVerticalBox>();
    Dialog->SetContent(Box(Tree, Modal, 670, 0));
    AddLine(Tree, *Modal, Accent, 2.f);
    auto* ModalSurface = Surface(Tree, Panel, FMargin(36), 1.5f);
    Modal->AddChildToVerticalBox(ModalSurface);
    auto* Body = Tree.ConstructWidget<UVerticalBox>();
    ModalSurface->SetContent(Body);
    DialogTitle = Label(Tree, FText(), 39, Family::Display, 700);
    Body->AddChildToVerticalBox(DialogTitle);
    DialogMessage = Label(Tree, FText(), 20, Family::Body, 500, Muted);
    DialogMessage->SetAutoWrapText(true);
    Body->AddChildToVerticalBox(DialogMessage)->SetPadding(FMargin(0, 8, 0, 28));
    auto* Choices = Tree.ConstructWidget<UHorizontalBox>();
    Body->AddChildToVerticalBox(Choices)->SetHorizontalAlignment(HAlign_Right);
    DialogCancel = FooterButton(TEXT("CancelSettingsDialog"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CancelDialog), false);
    DialogAccept = FooterButton(TEXT("AcceptSettingsDialog"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, AcceptDialog), true);
    Choices->AddChildToHorizontalBox(Box(Tree, DialogCancel, 0, 57))->SetPadding(FMargin(0, 0, 10, 0));
    Choices->AddChildToHorizontalBox(Box(Tree, DialogAccept, 0, 57));
    Dialog->SetVisibility(ESlateVisibility::Collapsed);
    StatusLabel = FooterStatus;
    ApplyButton = FooterApply;
    DiscardButton = FooterDiscard;
    SelectSection(0);
    return Super::RebuildWidget();
}

UButton* UDSettings::FooterButton(FName Name, const FText& Text, FName Action, bool bAccent)
{
    auto* Caption = Label(*WidgetTree, Text, 16, Family::Body, 600, bAccent ? Hex(0x23170E) : Ink);
    auto* Result = Button(*WidgetTree, *this, Name, Action, Caption);
    if (bAccent)
    {
        Result->SetStyle(AccentButtonStyle());
    }
    auto* CaptionSlot = CastChecked<UButtonSlot>(Caption->Slot);
    CaptionSlot->SetPadding(FMargin(18, 4));
    CaptionSlot->SetHorizontalAlignment(HAlign_Center);
    CaptionSlot->SetVerticalAlignment(VAlign_Center);
    return Result;
}

UHorizontalBox* UDSettings::AddRow(UVerticalBox& Page, const FText& Name)
{
    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    Page.AddChildToVerticalBox(Box(*WidgetTree, Row, 0, 80));
    AddLine(*WidgetTree, Page, Divider, 1.5f);
    auto* Caption = Row->AddChildToHorizontalBox(Label(*WidgetTree, Name, 18));
    Caption->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Caption->SetVerticalAlignment(VAlign_Center);
    return Row;
}

UWidget* UDSettings::OptionText(FString Value)
{
    return Label(*WidgetTree, FText::FromString(Value), 17);
}

void UDSettings::AddVolume(UVerticalBox& Page, FName Name, const FText& Title, int32 Index)
{
    auto* Row = AddRow(Page, Title);
    auto* Stack = WidgetTree->ConstructWidget<UOverlay>();
    auto* Track = WidgetTree->ConstructWidget<UProgressBar>();
    auto TrackStyle = Track->GetWidgetStyle();
    TrackStyle.BackgroundImage = FSlateColorBrush(Edge);
    TrackStyle.FillImage = FSlateColorBrush(FLinearColor::White);
    Track->SetWidgetStyle(TrackStyle);
    Track->SetFillColorAndOpacity(Accent);
    Track->SetBorderPadding(FVector2D::ZeroVector);
    Track->SetVisibility(ESlateVisibility::HitTestInvisible);
    Tracks.Add(Track);
    auto* TrackSlot = Stack->AddChildToOverlay(Box(*WidgetTree, Track, 0, 6));
    TrackSlot->SetHorizontalAlignment(HAlign_Fill);
    TrackSlot->SetVerticalAlignment(VAlign_Center);
    TrackSlot->SetPadding(FMargin(6, 0));
    auto* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
    auto Style = Slider->GetWidgetStyle();
    Style.NormalBarImage = Style.HoveredBarImage = FSlateColorBrush(FLinearColor::Transparent);
    Style.NormalThumbImage = FSlateRoundedBoxBrush(Accent, 2.f, FVector2f(15, 27));
    Style.HoveredThumbImage = FSlateRoundedBoxBrush(Ink, 2.f, FVector2f(15, 27));
    Slider->SetWidgetStyle(Style);
    Slider->SetIndentHandle(true);
    Slider->SetStepSize(.01f);
    if (Index == 0) { Slider->OnValueChanged.AddDynamic(this, &ThisClass::SetMaster); }
    else { Slider->OnValueChanged.AddDynamic(this, &ThisClass::SetMusic); }
    auto* SliderSlot = Stack->AddChildToOverlay(Slider);
    SliderSlot->SetHorizontalAlignment(HAlign_Fill);
    SliderSlot->SetVerticalAlignment(VAlign_Fill);
    Sliders.Add(Slider);
    Row->AddChildToHorizontalBox(Box(*WidgetTree, Stack, 200, 52))->SetVerticalAlignment(VAlign_Center);
    auto* Value = Label(*WidgetTree, FText(), 17, Family::Mono, 500, Muted);
    Value->SetJustification(ETextJustify::Right);
    Percentages.Add(Value);
    Row->AddChildToHorizontalBox(Box(*WidgetTree, Value, 75, 0))->SetVerticalAlignment(VAlign_Center);
}

void UDSettings::SelectSection(int32 Index)
{
    Sections->SetActiveWidgetIndex(Index);
    for (int32 Tab = 0; Tab < Tabs.Num(); ++Tab)
    {
        const bool bSelected = Index == Tab;
        Tabs[Tab]->SetStyle(ButtonStyle(FLinearColor::Transparent, FLinearColor::Transparent, 0.f));
        CastChecked<UTextBlock>(Tabs[Tab]->GetContent())->SetColorAndOpacity(bSelected ? Ink : Muted);
        Underlines[Tab]->SetBrushColor(bSelected ? Accent : FLinearColor::Transparent);
    }
}
void UDSettings::VideoTabClicked() { SelectSection(0); }
void UDSettings::AudioTabClicked() { SelectSection(1); }
void UDSettings::ControlsTabClicked() { SelectSection(2); }
UWidget* UDSettings::GetInitialFocus_Implementation() const { return Tabs.IsEmpty() ? nullptr : Tabs[0].Get(); }

void UDSettings::RefreshResolutions()
{
    const FIntPoint Current = DraftVideo.Resolution;
    if (ResolutionMode == DraftVideo.WindowMode && Resolutions.Contains(Current)) { return; }
    ResolutionMode = DraftVideo.WindowMode;
    Resolutions.Reset();
    if (ResolutionMode == EWindowMode::Fullscreen) { UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions); }
    else { UKismetSystemLibrary::GetConvenientWindowedResolutions(Resolutions); }
    Resolutions.AddUnique(Current);
    Resolutions.Sort([](FIntPoint A, FIntPoint B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
    Fields[1]->ClearOptions();
    for (FIntPoint Size : Resolutions) { Fields[1]->AddOption(ResolutionName(Size)); }
}

void UDSettings::RefreshOptions()
{
    TGuardValue<bool> Guard(bRefreshing, true);
    RefreshResolutions();
    Fields[0]->SetSelectedIndex(DraftVideo.WindowMode);
    Fields[1]->SetSelectedIndex(Resolutions.IndexOfByKey(DraftVideo.Resolution));
    if (DraftVideo.Quality < 0 && Fields[2]->GetOptionCount() == 5) { Fields[2]->AddOption(TEXT("Custom")); }
    if (DraftVideo.Quality >= 0 && Fields[2]->GetOptionCount() > 5) { Fields[2]->RemoveOption(TEXT("Custom")); }
    Fields[2]->SetSelectedIndex(DraftVideo.Quality < 0 ? 5 : DraftVideo.Quality);
    if (!FrameRates.Contains(DraftVideo.FrameRate))
    {
        FrameRates = { 0.f, 30.f, 60.f, 90.f, 120.f, 144.f, 240.f };
        FrameRates.AddUnique(DraftVideo.FrameRate);
        FrameRates.Sort();
        Fields[3]->ClearOptions();
        for (float Rate : FrameRates)
        {
            Fields[3]->AddOption(Rate > 0 ? FString::SanitizeFloat(Rate, 0) + TEXT(" FPS") : TEXT("Unlimited"));
        }
    }
    Fields[3]->SetSelectedIndex(FrameRates.IndexOfByKey(DraftVideo.FrameRate));
    Fields[4]->SetSelectedIndex(DraftVideo.bVSync ? 1 : 0);
    Fields[0]->SetIsEnabled(CanChangeDisplay());
    Fields[1]->SetIsEnabled(CanChangeDisplay() && DraftVideo.WindowMode != EWindowMode::WindowedFullscreen);
    Fields[0]->GetParent()->SetIsEnabled(Fields[0]->GetIsEnabled());
    Fields[1]->GetParent()->SetIsEnabled(Fields[1]->GetIsEnabled());
    for (int32 Index = 0; Index < Sliders.Num(); ++Index)
    {
        Sliders[Index]->SetValue(DraftAudio[Index]);
        Tracks[Index]->SetPercent(DraftAudio[Index]);
        Percentages[Index]->SetText(FText::AsPercent(DraftAudio[Index]));
    }
    FooterApply->SetIsEnabled(HasDraft());
    FooterDiscard->SetIsEnabled(HasDraft());
    if (IsDisplayPreview() && !bDisplayDialog) { ShowDialog(true); }
    else if (!IsDisplayPreview() && bDisplayDialog) { HideDialog(); }
}

void UDSettings::SelectionChanged(FString Value, ESelectInfo::Type Type)
{
    if (bRefreshing || Value.IsEmpty() || FrameRates.IsEmpty()) { return; }
    DraftVideo.WindowMode = Fields[0]->GetSelectedIndex();
    const int32 Resolution = Fields[1]->GetSelectedIndex();
    if (Resolutions.IsValidIndex(Resolution)) { DraftVideo.Resolution = Resolutions[Resolution]; }
    const int32 Quality = Fields[2]->GetSelectedIndex();
    DraftVideo.Quality = Quality == 5 ? -1 : Quality;
    const int32 Rate = Fields[3]->GetSelectedIndex();
    if (FrameRates.IsValidIndex(Rate)) { DraftVideo.FrameRate = FrameRates[Rate]; }
    DraftVideo.bVSync = Fields[4]->GetSelectedIndex() == 1;
    FooterStatus->SetText(FText());
    RefreshOptions();
}

void UDSettings::SetVolume(int32 Index, float Value)
{
    if (bRefreshing) { return; }
    DraftAudio[Index] = FMath::RoundToFloat(FMath::Clamp(Value, 0.f, 1.f) * 100.f) / 100.f;
    FooterStatus->SetText(FText());
    RefreshOptions();
}

void UDSettings::SetMaster(float Value) { SetVolume(0, Value); }
void UDSettings::SetMusic(float Value) { SetVolume(1, Value); }

void UDSettings::ShowDialog(bool bDisplay)
{
    bDisplayDialog = bDisplay;
    DialogTitle->SetText(bDisplay ? LOCTEXT("Keep", "Keep Display Settings?") : LOCTEXT("Unsaved", "Unsaved Changes"));
    DialogMessage->SetText(bDisplay ? LOCTEXT("Reverting", "The previous display settings will be restored in 15 s.")
        : LOCTEXT("DiscardQuestion", "Discard your changes and return?"));
    ButtonCaption(DialogCancel, bDisplay ? LOCTEXT("Revert", "Revert") : LOCTEXT("Cancel", "Cancel"));
    ButtonCaption(DialogAccept, bDisplay ? LOCTEXT("KeepButton", "Keep Changes") : LOCTEXT("DiscardButton", "Discard Changes"));
    PanelBody->SetIsEnabled(false);
    Dialog->SetVisibility(ESlateVisibility::Visible);
    if (bDisplay)
    {
        StatusLabel = DialogMessage;
        DiscardButton = DialogCancel;
    }
    DialogCancel->SetUserFocus(GetOwningPlayer());
}

void UDSettings::HideDialog()
{
    bDisplayDialog = false;
    StatusLabel = FooterStatus;
    DiscardButton = FooterDiscard;
    Dialog->SetVisibility(ESlateVisibility::Collapsed);
    PanelBody->SetIsEnabled(true);
    BackButton->SetUserFocus(GetOwningPlayer());
}

void UDSettings::CancelDialog()
{
    if (bDisplayDialog) { Discard(); }
    else { HideDialog(); }
}

void UDSettings::AcceptDialog()
{
    if (bDisplayDialog) { Apply(); }
    else
    {
        HideDialog();
        Discard();
        Super::Back();
    }
}

void UDSettings::Back()
{
    if (Dialog->GetVisibility() == ESlateVisibility::Visible) { CancelDialog(); }
    else if (HasDraft()) { ShowDialog(false); }
    else { Super::Back(); }
}
#undef LOCTEXT_NAMESPACE
