#include "CoreKitFrontend/CoreKitFrontendMenu.h"

#include "Algo/Find.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "CoreKitFrontend/CoreKitFrontendSettings.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "CoreKitNavigation/CoreKitKeyBindings.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

#define LOCTEXT_NAMESPACE "CoreKitFrontendMenu"

namespace
{
const UCoreKitFrontendSettings& Config()
{
	return *GetDefault<UCoreKitFrontendSettings>();
}

UTextBlock* MakeLabel(UWidgetTree& Tree, const FText& Text, int32 FontSize)
{
	auto* Label = Tree.ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = FontSize;
	Label->SetFont(Font);
	Label->SetText(Text);
	Label->SetAutoWrapText(true);
	return Label;
}

void SetButtonText(UButton* Button, const FText& Text)
{
	CastChecked<UTextBlock>(Button->GetContent())->SetText(Text);
}

FMapPlayerKeyArgs MappingArgs(const FCoreKitFrontendBinding& Binding)
{
	FMapPlayerKeyArgs Args;
	Args.MappingName = Binding.MappingName;
	Args.Slot = static_cast<EPlayerMappableKeySlot>(FMath::Clamp(Binding.Slot, 0, static_cast<int32>(EPlayerMappableKeySlot::Seventh)));
	Args.HardwareDeviceId = Binding.HardwareDeviceId;
	return Args;
}

UEnhancedInputUserSettings* GetInputSettings(ULocalPlayer* Player)
{
	auto* Input = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return Input ? Input->GetUserSettings() : nullptr;
}
}

UCoreKitFrontendSubsystem* UCoreKitFrontendMenu::GetFrontend() const
{
	auto* Game = GetGameInstance();
	return Game ? Game->GetSubsystem<UCoreKitFrontendSubsystem>() : nullptr;
}

TSharedRef<SWidget> UCoreKitFrontendMenu::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		auto* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetBrushColor(FLinearColor(.02f, .033f, .06f));
		Background->SetPadding(FMargin(16.f));
		Background->SetHorizontalAlignment(HAlign_Center);
		Background->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Background;
		auto* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetMinDesiredWidth(380.f);
		Background->SetContent(Width);
		auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Width->SetContent(Column);
		Column->AddChildToVerticalBox(MakeLabel(*WidgetTree, LOCTEXT("Title", "MAIN MENU"), 27))->SetPadding(FMargin(0, 0, 0, 12));
		Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
		Column->AddChildToVerticalBox(Pages);
		auto* Main = WidgetTree->ConstructWidget<UVerticalBox>();
		Pages->AddChild(Main);
		NewButton = AddButton(*Main, TEXT("NewGame"), LOCTEXT("New", "New Game"), GET_FUNCTION_NAME_CHECKED(ThisClass, NewGame));
		ContinueButton = AddButton(*Main, TEXT("ContinueGame"), LOCTEXT("Continue", "Continue"), GET_FUNCTION_NAME_CHECKED(ThisClass, ContinueGame));
		SettingsButton = AddButton(*Main, TEXT("OpenSettings"), LOCTEXT("Settings", "Settings"), GET_FUNCTION_NAME_CHECKED(ThisClass, OpenSettings));
		auto* Height = WidgetTree->ConstructWidget<USizeBox>();
		Height->SetMaxDesiredHeight(480.f);
		Pages->AddChild(Height);
		auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
		Height->SetContent(Scroll);
		auto* Settings = WidgetTree->ConstructWidget<UVerticalBox>();
		Scroll->AddChild(Settings);
		BuildSettings(*Settings);
		StatusLabel = MakeLabel(*WidgetTree, FText::GetEmpty(), 13);
		Column->AddChildToVerticalBox(StatusLabel)->SetPadding(FMargin(0, 12, 0, 0));
	}
	return Super::RebuildWidget();
}

UButton* UCoreKitFrontendMenu::AddButton(UVerticalBox& Page, FName Name, const FText& Text, FName Callback)
{
	auto* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	Button->SetContent(MakeLabel(*WidgetTree, Text, 15));
	Page.AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 3));
	FScriptDelegate Clicked;
	Clicked.BindUFunction(this, Callback);
	Button->OnClicked.Add(Clicked);
	return Button;
}

void UCoreKitFrontendMenu::BuildSettings(UVerticalBox& Page)
{
	Page.AddChildToVerticalBox(MakeLabel(*WidgetTree, LOCTEXT("Video", "Video"), 19));
	OptionButtons = {
		AddButton(Page, TEXT("WindowMode"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleWindowMode)),
		AddButton(Page, TEXT("Resolution"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleResolution)),
		AddButton(Page, TEXT("Quality"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleQuality)),
		AddButton(Page, TEXT("FrameRate"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleFrameRate)),
		AddButton(Page, TEXT("VSync"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, ToggleVSync))
	};

	const auto& Settings = Config();
	if (Settings.UserVolumeMix && (Settings.MasterSoundClass || Settings.MusicSoundClass || Settings.EffectsSoundClass))
	{
		Page.AddChildToVerticalBox(MakeLabel(*WidgetTree, LOCTEXT("Audio", "Audio"), 19))->SetPadding(FMargin(0, 8, 0, 0));
		AudioButtons.SetNum(3);
		if (Settings.MasterSoundClass)
		{
			AudioButtons[0] = AddButton(Page, TEXT("MasterVolume"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleMaster));
		}
		if (Settings.MusicSoundClass)
		{
			AudioButtons[1] = AddButton(Page, TEXT("MusicVolume"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleMusic));
		}
		if (Settings.EffectsSoundClass)
		{
			AudioButtons[2] = AddButton(Page, TEXT("EffectsVolume"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, CycleEffects));
		}
	}
	if (!Settings.EditableBindings.IsEmpty())
	{
		Page.AddChildToVerticalBox(MakeLabel(*WidgetTree, LOCTEXT("Controls", "Controls"), 19))->SetPadding(FMargin(0, 8, 0, 0));
		ControlButtons = {
			AddButton(Page, TEXT("SelectedBinding"), {}, GET_FUNCTION_NAME_CHECKED(ThisClass, SelectBinding)),
			AddButton(Page, TEXT("Rebind"), LOCTEXT("Rebind", "Rebind selected action"), GET_FUNCTION_NAME_CHECKED(ThisClass, CaptureBinding)),
			AddButton(Page, TEXT("ResetBinding"), LOCTEXT("ResetBinding", "Reset selected action on all devices"), GET_FUNCTION_NAME_CHECKED(ThisClass, ResetBindings))
		};
	}
	ApplyButton = AddButton(Page, TEXT("Apply"), LOCTEXT("Apply", "Apply"), GET_FUNCTION_NAME_CHECKED(ThisClass, Apply));
	DiscardButton = AddButton(Page, TEXT("Discard"), LOCTEXT("Discard", "Discard"), GET_FUNCTION_NAME_CHECKED(ThisClass, Discard));
	BackButton = AddButton(Page, TEXT("Back"), LOCTEXT("Back", "Back"), GET_FUNCTION_NAME_CHECKED(ThisClass, Back));
}

void UCoreKitFrontendMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (auto* Frontend = GetFrontend())
	{
		Frontend->OnChanged.AddUniqueDynamic(this, &ThisClass::RefreshPage);
		RefreshPage();
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &ThisClass::HandleAppActivation);
	}
}

void UCoreKitFrontendMenu::NativeDestruct()
{
	CancelCapture();
	ResetDisplayPreview();
	if (auto* Frontend = GetFrontend())
	{
		Frontend->OnChanged.RemoveDynamic(this, &ThisClass::RefreshPage);
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().RemoveAll(this);
	}
	Super::NativeDestruct();
}

UWidget* UCoreKitFrontendMenu::GetInitialFocus_Implementation() const
{
	if (bSettingsOpen && BackButton)
	{
		return OptionButtons[0]->GetIsEnabled() ? OptionButtons[0].Get() : OptionButtons[2].Get();
	}
	return NewButton;
}

void UCoreKitFrontendMenu::RefreshPage()
{
	const auto* Frontend = GetFrontend();
	const bool bShowingSettings = Frontend->GetState() == ECoreKitFrontendState::Settings;
	const bool bWasSettings = bSettingsOpen;
	bSettingsOpen = bShowingSettings;
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(bSettingsOpen ? 1 : 0);
		NewButton->SetIsEnabled(Frontend->GetState() == ECoreKitFrontendState::MainMenu);
		SettingsButton->SetIsEnabled(NewButton->GetIsEnabled());
		ContinueButton->SetIsEnabled(Frontend->CanContinue());
		if (!bSettingsOpen || !bWasSettings)
		{
			StatusLabel->SetText(Frontend->GetStatus());
		}
		if (bSettingsOpen && !bWasSettings)
		{
			ReadOptions();
		}
		else if (bWasSettings && !bSettingsOpen)
		{
			CancelCapture();
			ResetDisplayPreview();
		}
	}
	OnFrontendUpdated();
}

void UCoreKitFrontendMenu::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (PreviewDeadline > 0.0)
	{
		const double Remaining = PreviewDeadline - FPlatformTime::Seconds();
		if (Remaining <= 0.0)
		{
			ResetDisplayPreview();
			SetOptionsStatus(LOCTEXT("PreviewExpired", "Display settings reverted."));
			BackButton->SetUserFocus(GetOwningPlayer());
		}
		else
		{
			SetOptionsStatus(FText::Format(LOCTEXT("ConfirmCountdown", "Keep this display mode? Reverting in {0} s."),
				FText::AsNumber(FMath::CeilToInt(Remaining))));
		}
	}
}

FReply UCoreKitFrontendMenu::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (bSettingsOpen && Pages && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
	{
		Back();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

void UCoreKitFrontendMenu::HandleAppActivation(bool bActive)
{
	if (!bActive)
	{
		CancelCapture();
		if (PreviewDeadline > 0.0)
		{
			ResetDisplayPreview();
			SetOptionsStatus(LOCTEXT("PreviewInactive", "Display settings reverted after focus changed."));
		}
	}
}

void UCoreKitFrontendMenu::NewGame() { GetFrontend()->NewGame(); }
void UCoreKitFrontendMenu::ContinueGame() { GetFrontend()->ContinueGame(); }
void UCoreKitFrontendMenu::OpenSettings() { GetFrontend()->OpenSettings(); }

void UCoreKitFrontendMenu::Back()
{
	if (PreviewDeadline > 0.0 || bCapturing)
	{
		Discard();
	}
	else if (HasDraft())
	{
		SetOptionsStatus(LOCTEXT("DirtyBack", "Apply or discard your changes before going back."));
	}
	else if (auto* Frontend = GetFrontend())
	{
		Frontend->CloseSettings();
		SettingsButton->SetUserFocus(GetOwningPlayer());
	}
}

void UCoreKitFrontendMenu::ReadOptions()
{
	if (const auto* Settings = UGameUserSettings::GetGameUserSettings())
	{
		CommittedVideo = { Settings->GetScreenResolution(), static_cast<int32>(Settings->GetFullscreenMode()),
			Settings->GetOverallScalabilityLevel(), Settings->GetFrameRateLimit(), Settings->IsVSyncEnabled() };
	}
	CommittedAudio = GetFrontend()->AudioVolumes;
	DraftVideo = CommittedVideo;
	DraftAudio = CommittedAudio;
	if (auto* Input = GetInputSettings(GetOwningLocalPlayer()))
	{
		for (const FCoreKitFrontendBinding& Binding : Config().EditableBindings)
		{
			if (auto* Context = Binding.Context.LoadSynchronous())
			{
				Input->RegisterInputMappingContext(Context);
			}
		}
		for (const auto& Reference : Config().ConcurrentBindingContexts)
		{
			if (auto* Context = Reference.LoadSynchronous())
			{
				Input->RegisterInputMappingContext(Context);
			}
		}
	}
	RefreshOptions();
}

bool UCoreKitFrontendMenu::CanChangeDisplay() const
{
	return GetWorld() && GetWorld()->WorldType != EWorldType::PIE;
}

bool UCoreKitFrontendMenu::HasDraft() const
{
	return DraftVideo != CommittedVideo || DraftAudio != CommittedAudio;
}

void UCoreKitFrontendMenu::RefreshOptions()
{
	if (OptionButtons.IsEmpty())
	{
		return;
	}
	const bool bPreview = PreviewDeadline > 0.0;
	const bool bEditable = !bPreview && !bCapturing;
	const FText WindowModes[] = { LOCTEXT("Fullscreen", "Fullscreen"), LOCTEXT("Borderless", "Borderless"), LOCTEXT("Windowed", "Windowed") };
	const FText Quality[] = { LOCTEXT("Low", "Low"), LOCTEXT("Medium", "Medium"), LOCTEXT("High", "High"), LOCTEXT("Epic", "Epic"), LOCTEXT("Cinematic", "Cinematic") };
	const int32 Mode = FMath::Clamp(DraftVideo.WindowMode, 0, 2);
	SetButtonText(OptionButtons[0], FText::Format(LOCTEXT("WindowModeValue", "Window mode  {0}"), WindowModes[Mode]));
	SetButtonText(OptionButtons[1], FText::Format(LOCTEXT("ResolutionValue", "Resolution  {0} × {1}"),
		FText::AsNumber(DraftVideo.Resolution.X, &FNumberFormattingOptions::DefaultNoGrouping()),
		FText::AsNumber(DraftVideo.Resolution.Y, &FNumberFormattingOptions::DefaultNoGrouping())));
	SetButtonText(OptionButtons[2], FText::Format(LOCTEXT("QualityValue", "Graphics quality  {0}"),
		DraftVideo.Quality >= 0 && DraftVideo.Quality < UE_ARRAY_COUNT(Quality) ? Quality[DraftVideo.Quality] : LOCTEXT("Custom", "Custom")));
	SetButtonText(OptionButtons[3], FText::Format(LOCTEXT("FrameRateValue", "Frame limit  {0}"),
		DraftVideo.FrameRate <= 0.f ? LOCTEXT("Unlimited", "Unlimited") : FText::AsNumber(DraftVideo.FrameRate)));
	SetButtonText(OptionButtons[4], DraftVideo.bVSync ? LOCTEXT("VSyncOn", "VSync  On") : LOCTEXT("VSyncOff", "VSync  Off"));
	OptionButtons[0]->SetIsEnabled(bEditable && CanChangeDisplay());
	OptionButtons[1]->SetIsEnabled(bEditable && CanChangeDisplay() && Mode != EWindowMode::WindowedFullscreen);
	for (int32 Index = 2; Index < OptionButtons.Num(); ++Index)
	{
		OptionButtons[Index]->SetIsEnabled(bEditable);
	}
	const FText AudioLabels[] = { LOCTEXT("Master", "Master"), LOCTEXT("Music", "Music"), LOCTEXT("Effects", "Effects") };
	for (int32 Index = 0; Index < AudioButtons.Num(); ++Index)
	{
		if (UButton* Button = AudioButtons[Index])
		{
			SetButtonText(Button, FText::Format(LOCTEXT("VolumeValue", "{0}  {1}"), AudioLabels[Index], FText::AsPercent(DraftAudio[Index])));
			Button->SetIsEnabled(bEditable);
		}
	}
	SetButtonText(ApplyButton, bPreview ? LOCTEXT("KeepDisplay", "Keep display settings") : LOCTEXT("Apply", "Apply"));
	SetButtonText(DiscardButton, bPreview ? LOCTEXT("RevertDisplay", "Revert display settings") : LOCTEXT("Discard", "Discard"));
	ApplyButton->SetIsEnabled(!bCapturing && (HasDraft() || bPreview));
	DiscardButton->SetIsEnabled(HasDraft() || bPreview || bCapturing);
	RefreshControls();
}

void UCoreKitFrontendMenu::SetOptionsStatus(const FText& Message)
{
	if (StatusLabel)
	{
		StatusLabel->SetText(Message);
	}
}

void UCoreKitFrontendMenu::CycleWindowMode()
{
	DraftVideo.WindowMode = (DraftVideo.WindowMode + 1) % 3;
	RefreshOptions();
}

void UCoreKitFrontendMenu::CycleResolution()
{
	TArray<FIntPoint> Resolutions;
	if (DraftVideo.WindowMode == EWindowMode::Fullscreen)
	{
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	}
	else
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(Resolutions);
	}
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X == B.X ? A.Y < B.Y : A.X < B.X; });
	if (!Resolutions.IsEmpty())
	{
		DraftVideo.Resolution = Resolutions[(Resolutions.IndexOfByKey(DraftVideo.Resolution) + 1) % Resolutions.Num()];
		RefreshOptions();
	}
}

void UCoreKitFrontendMenu::CycleQuality()
{
	DraftVideo.Quality = (DraftVideo.Quality + 1) % 5;
	RefreshOptions();
}

void UCoreKitFrontendMenu::CycleFrameRate()
{
	const float Rates[] = { 0.f, 30.f, 60.f, 90.f, 120.f, 144.f, 240.f };
	const float* Next = Algo::FindByPredicate(Rates, [this](float Rate) { return Rate > DraftVideo.FrameRate; });
	DraftVideo.FrameRate = Next ? *Next : Rates[0];
	RefreshOptions();
}

void UCoreKitFrontendMenu::ToggleVSync()
{
	DraftVideo.bVSync = !DraftVideo.bVSync;
	RefreshOptions();
}

void UCoreKitFrontendMenu::AdvanceVolume(int32 Index)
{
	DraftAudio[Index] = static_cast<float>((FMath::RoundToInt(DraftAudio[Index] * 10.f) + 1) % 11) / 10.f;
	RefreshOptions();
}

void UCoreKitFrontendMenu::CycleMaster() { AdvanceVolume(0); }
void UCoreKitFrontendMenu::CycleMusic() { AdvanceVolume(1); }
void UCoreKitFrontendMenu::CycleEffects() { AdvanceVolume(2); }

void UCoreKitFrontendMenu::Apply()
{
	auto* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings || !HasDraft())
	{
		return;
	}
	if (PreviewDeadline <= 0.0 && CanChangeDisplay()
		&& (DraftVideo.Resolution != CommittedVideo.Resolution || DraftVideo.WindowMode != CommittedVideo.WindowMode))
	{
		Settings->SetScreenResolution(DraftVideo.Resolution);
		Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(DraftVideo.WindowMode));
		Settings->ApplyResolutionSettings(false);
		PreviewDeadline = FPlatformTime::Seconds() + 15.0;
		RefreshOptions();
		DiscardButton->SetUserFocus(GetOwningPlayer());
		return;
	}
	CommitOptions();
}

void UCoreKitFrontendMenu::CommitOptions()
{
	auto* Settings = UGameUserSettings::GetGameUserSettings();
	if (PreviewDeadline > 0.0)
	{
		Settings->ConfirmVideoMode();
		PreviewDeadline = 0.0;
	}
	if (DraftVideo != CommittedVideo)
	{
		if (DraftVideo.Quality != CommittedVideo.Quality)
		{
			Settings->SetOverallScalabilityLevel(DraftVideo.Quality);
		}
		Settings->SetVSyncEnabled(DraftVideo.bVSync);
		Settings->SetFrameRateLimit(DraftVideo.FrameRate);
		Settings->ApplySettings(false);
	}
	if (DraftAudio != CommittedAudio)
	{
		GetFrontend()->SaveAudio(DraftAudio);
	}
	ReadOptions();
	SetOptionsStatus(LOCTEXT("SettingsApplied", "Settings applied."));
	BackButton->SetUserFocus(GetOwningPlayer());
}

void UCoreKitFrontendMenu::ResetDisplayPreview()
{
	if (PreviewDeadline <= 0.0)
	{
		return;
	}
	PreviewDeadline = 0.0;
	if (auto* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetScreenResolution(CommittedVideo.Resolution);
		Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(CommittedVideo.WindowMode));
		Settings->ApplyResolutionSettings(false);
	}
	DraftVideo.Resolution = CommittedVideo.Resolution;
	DraftVideo.WindowMode = CommittedVideo.WindowMode;
	RefreshOptions();
}

void UCoreKitFrontendMenu::Discard()
{
	if (bCapturing)
	{
		CancelCapture();
	}
	else if (PreviewDeadline > 0.0)
	{
		ResetDisplayPreview();
		SetOptionsStatus(LOCTEXT("DisplayReverted", "Display settings reverted."));
	}
	else
	{
		DraftVideo = CommittedVideo;
		DraftAudio = CommittedAudio;
		RefreshOptions();
		SetOptionsStatus(LOCTEXT("Discarded", "Unapplied changes discarded."));
	}
	BackButton->SetUserFocus(GetOwningPlayer());
}

void UCoreKitFrontendMenu::RefreshControls()
{
	if (ControlButtons.IsEmpty())
	{
		return;
	}
	const auto& Binding = Config().EditableBindings[BindingIndex];
	const auto* Settings = GetInputSettings(GetOwningLocalPlayer());
	const auto* Profile = Settings ? Settings->GetActiveKeyProfile() : nullptr;
	const auto* Mapping = Profile ? Profile->FindKeyMapping(MappingArgs(Binding)) : nullptr;
	const FText Label = Binding.Label.IsEmpty() ? FText::FromName(Binding.MappingName) : Binding.Label;
	SetButtonText(ControlButtons[0], FText::Format(LOCTEXT("BindingValue", "{0}  {1}"), Label,
		Mapping ? Mapping->GetCurrentKey().GetDisplayName() : LOCTEXT("Unmapped", "Unavailable")));
	ControlButtons[0]->SetIsEnabled(PreviewDeadline <= 0.0 && !bCapturing);
	const bool bEditable = Mapping && Mapping->GetCurrentKey().IsGamepadKey() == Binding.bGamepad
		&& Binding.Context.IsValid() && PreviewDeadline <= 0.0 && !bCapturing;
	ControlButtons[1]->SetIsEnabled(bEditable);
	ControlButtons[2]->SetIsEnabled(bEditable);
}

void UCoreKitFrontendMenu::SelectBinding()
{
	BindingIndex = (BindingIndex + 1) % Config().EditableBindings.Num();
	RefreshControls();
}

void UCoreKitFrontendMenu::CaptureBinding()
{
	auto* Nav = GetOwningLocalPlayer()->GetSubsystem<UCoreKitNavigationSubsystem>();
	if (Nav->BeginKeyCapture())
	{
		bCapturing = true;
		Nav->OnKeyCaptureFinished.AddUniqueDynamic(this, &ThisClass::KeyCaptured);
		SetOptionsStatus(LOCTEXT("Capture", "Press a new key. Escape or the controller Menu button cancels. Accepted bindings save immediately."));
		RefreshOptions();
	}
}

void UCoreKitFrontendMenu::CancelCapture()
{
	if (bCapturing)
	{
		if (auto* Player = GetOwningLocalPlayer())
		{
			Player->GetSubsystem<UCoreKitNavigationSubsystem>()->CancelKeyCapture();
		}
		bCapturing = false;
	}
}

void UCoreKitFrontendMenu::KeyCaptured(bool bCaptured, FKey Key)
{
	auto* Player = GetOwningLocalPlayer();
	if (!Player)
	{
		bCapturing = false;
		return;
	}
	Player->GetSubsystem<UCoreKitNavigationSubsystem>()->OnKeyCaptureFinished.RemoveDynamic(this, &ThisClass::KeyCaptured);
	bCapturing = false;
	if (bCaptured && Key != EKeys::Gamepad_Special_Right)
	{
		const auto& Binding = Config().EditableBindings[BindingIndex];
		FMapPlayerKeyArgs Args = MappingArgs(Binding);
		Args.NewKey = Key;
		TArray<UInputMappingContext*> Concurrent;
		for (const auto& Context : Config().ConcurrentBindingContexts)
		{
			Concurrent.Add(Context.Get());
		}
		const auto Result = UCoreKitKeyBindings::ApplyKeyBinding(Player, Binding.Context.Get(), Args, Concurrent,
			Binding.bGamepad ? ECoreKitBindingDevice::Gamepad : ECoreKitBindingDevice::KeyboardAndMouse).Result;
		if (Result == ECoreKitBindingResult::Success)
		{
			SetOptionsStatus(LOCTEXT("BindingApplied", "Binding updated."));
		}
		else
		{
			SetOptionsStatus(Result == ECoreKitBindingResult::Conflict
				? LOCTEXT("BindingConflict", "That key is already assigned in a concurrent context.")
				: LOCTEXT("BindingRejected", "That key is unavailable for this binding."));
		}
	}
	else
	{
		SetOptionsStatus(LOCTEXT("CaptureCancelled", "Key capture cancelled."));
	}
	RefreshOptions();
	if (ControlButtons[1]->GetIsEnabled())
	{
		ControlButtons[1]->SetUserFocus(GetOwningPlayer());
	}
}

void UCoreKitFrontendMenu::ResetBindings()
{
	const auto Result = UCoreKitKeyBindings::RestoreBindingDefaults(GetOwningLocalPlayer(), MappingArgs(Config().EditableBindings[BindingIndex]));
	SetOptionsStatus(Result == ECoreKitBindingResult::Success ? LOCTEXT("BindingReset", "Default bindings restored.")
		: LOCTEXT("BindingResetFailed", "The bindings could not be restored."));
	RefreshControls();
}

#undef LOCTEXT_NAMESPACE
