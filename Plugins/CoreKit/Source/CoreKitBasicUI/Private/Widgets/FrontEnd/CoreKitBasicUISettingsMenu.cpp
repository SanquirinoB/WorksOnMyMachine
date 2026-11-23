#include "Widgets/FrontEnd/CoreKitBasicUIFrontEndCommon.h"

#include "Blueprint/WidgetTree.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "GameFramework/GameUserSettings.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitNavigation/CoreKitKeyBindings.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitNavigation/CoreKitOptionSelector.h"
#include "CoreKitNavigation/CoreKitSectionList.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUIUserSettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUISettings"

// ---------------------------------------------------------------------------------------------------------------
// Draft

void UCoreKitBasicUISettingsDraft::CopySettings(UCoreKitBasicUIUserSettings* To, const UCoreKitBasicUIUserSettings* From)
{
	if (!To || !From)
	{
		return;
	}
	for (TFieldIterator<FProperty> It(UCoreKitBasicUIUserSettings::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Config)) It->CopyCompleteValue_InContainer(To, From);
	}
}

bool UCoreKitBasicUISettingsDraft::SameVideo(const UCoreKitBasicUIUserSettings* A, const UCoreKitBasicUIUserSettings* B)
{
	return A && B && A->DisplayMode == B->DisplayMode && A->Resolution == B->Resolution && A->FrameRateLimit == B->FrameRateLimit && A->bVSync == B->bVSync;
}

void UCoreKitBasicUISettingsDraft::Begin(UCoreKitBasicUIUserSettings* InTarget)
{
	Target = InTarget;
	Committed = NewObject<UCoreKitBasicUIUserSettings>(this, NAME_None, RF_Transient);
	CopySettings(Committed, Target);
}

bool UCoreKitBasicUISettingsDraft::IsDirty() const
{
	if (!Target || !Committed)
	{
		return false;
	}
	for (TFieldIterator<FProperty> It(UCoreKitBasicUIUserSettings::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Config) && !It->Identical_InContainer(Target, Committed))
		{
			return true;
		}
	}
	return false;
}

bool UCoreKitBasicUISettingsDraft::IsVideoDirty() const
{
	return Target && Committed && !SameVideo(Target, Committed);
}

void UCoreKitBasicUISettingsDraft::Preview(const UObject* WorldContext)
{
	// Gameplay, HUD, audio and motion blur preview live; display mode, resolution, frame limit and V-Sync wait for
	// APPLY (re-applying the window on every slider step would flicker).
	if (Target && bApplyToEngine)
	{
		Target->ApplyNonVideo(WorldContext);
	}
}

void UCoreKitBasicUISettingsDraft::Commit(const UObject* WorldContext)
{
	if (!Target)
	{
		return;
	}
	if (bApplyToEngine)
	{
		Target->Apply(WorldContext);
	}
	if (bSaveOnCommit)
	{
		Target->Save();
	}
	CopySettings(Committed, Target);
}

void UCoreKitBasicUISettingsDraft::Discard(const UObject* WorldContext)
{
	CopySettings(Target, Committed);
	if (Target && bApplyToEngine)
	{
		// Video values were never applied live, so only the live part needs restoring.
		Target->ApplyNonVideo(WorldContext);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Row inventory (Docs/UI/Spec/05-FrontEnd.md § Settings)

namespace
{
	enum ETab : int32 { Gameplay, Controls, Video, Audio, TabCount };

	FCoreKitBasicUISettingsRowInfo Info(const TCHAR* Id, const FText& Label, ECoreKitBasicUISettingsControl Control, const FText& Description, const TCHAR* Action = nullptr)
	{
		FCoreKitBasicUISettingsRowInfo Row;
		Row.Id = Id;
		Row.Label = Label;
		Row.Control = Control;
		Row.Description = Description;
		Row.Action = Action ? FName(Action) : NAME_None;
		return Row;
	}

	const FText& RebindText()
	{
		static const FText Text = COREKIT_BASICUI_TEXT("RebindBody", "Press any key or mouse button to assign. A key already in use swaps with this action. Escape cancels.");
		return Text;
	}

	const int32 FrameRates[] = { 30, 60, 120, 144, 240, 0 };

	bool IsKey(const FKey& Key, std::initializer_list<FKey> Keys)
	{
		for (const FKey& Candidate : Keys)
		{
			if (Key == Candidate)
			{
				return true;
			}
		}
		return false;
	}
}

const TArray<FCoreKitBasicUISettingsRowInfo>& UCoreKitBasicUISettingsMenu::GetRowInfos(int32 Tab)
{
	using C = ECoreKitBasicUISettingsControl;
	static const TArray<FCoreKitBasicUISettingsRowInfo> Tabs[TabCount] = {
		{
			Info(TEXT("MouseSensitivity"), COREKIT_BASICUI_TEXT("Sensitivity", "Mouse sensitivity"), C::Slider,
				COREKIT_BASICUI_TEXT("SensitivityBody", "How fast the camera turns. Lower values make long rifle tracking steadier; higher values make it easier to turn on drones behind you.")),
			Info(TEXT("InvertLookY"), COREKIT_BASICUI_TEXT("Invert", "Invert look Y"), C::Segmented,
				COREKIT_BASICUI_TEXT("InvertBody", "Flips vertical look: pushing the mouse or stick forward looks down. Leave it off unless your hands already expect flight controls.")),
			Info(TEXT("FieldOfView"), COREKIT_BASICUI_TEXT("FOV", "Field of view"), C::Slider,
				COREKIT_BASICUI_TEXT("FOVBody", "How much of the arena fits on screen. Wider shows drones closing in from the sides; narrower makes far targets bigger for the rifle.")),
			Info(TEXT("DashNoInput"), COREKIT_BASICUI_TEXT("DashNoInput", "Dash with no input"), C::Segmented,
				COREKIT_BASICUI_TEXT("DashNoInputBody", "Where Dash goes when no direction is held. Forward closes the gap for the katana; backward pulls you out of a slam ring.")),
			Info(TEXT("DamageNumbers"), COREKIT_BASICUI_TEXT("DamageNumbers", "Damage numbers"), C::Segmented,
				COREKIT_BASICUI_TEXT("DamageNumbersBody", "Shows the damage of every hit above the target. Off keeps the screen clean; hit markers and kill confirms stay.")),
			Info(TEXT("ScreenShake"), COREKIT_BASICUI_TEXT("Shake", "Screen shake"), C::Slider,
				COREKIT_BASICUI_TEXT("ShakeBody", "Camera shake from hits taken, Dash and the Heavy Mech's slam. Zero turns it off; Reduced motion turns it off too.")),
			Info(TEXT("HudScale"), COREKIT_BASICUI_TEXT("HudScale", "HUD scale"), C::Slider,
				COREKIT_BASICUI_TEXT("HudScaleBody", "Size of every HUD block. The HUD stays inside the safe area at any scale.")),
			Info(TEXT("Crosshair"), COREKIT_BASICUI_TEXT("Crosshair", "Crosshair colour"), C::Swatches,
				COREKIT_BASICUI_TEXT("CrosshairBody", "Colour of the crosshair dot and ticks. Pick the one that reads best against the arena; hit markers keep the colour of the active weapon.")),
			Info(TEXT("ReducedMotion"), COREKIT_BASICUI_TEXT("ReducedMotion", "Reduced motion"), C::Segmented,
				COREKIT_BASICUI_TEXT("ReducedMotionBody", "Turns off screen shake, glitch, the FOV kick, speed lines and card parallax. Fades and colour changes stay, so nothing is lost.")),
			Info(TEXT("GamepadGlyphs"), COREKIT_BASICUI_TEXT("Glyphs", "Gamepad glyphs"), C::Segmented,
				COREKIT_BASICUI_TEXT("GlyphsBody", "Which button icons a gamepad shows. AUTO follows the connected pad; pick XBOX or PLAYSTATION when your pad is shown wrong.")),
		},
		{
			Info(TEXT("Dash"), COREKIT_BASICUI_TEXT("BindDash", "Dash"), C::Binding, RebindText(), TEXT("IA_Dash")),
			Info(TEXT("Jump"), COREKIT_BASICUI_TEXT("BindJump", "Jump"), C::Binding, RebindText(), TEXT("IA_Jump")),
			Info(TEXT("Reload"), COREKIT_BASICUI_TEXT("BindReload", "Reload"), C::Binding, RebindText(), TEXT("IA_Reload")),
			Info(TEXT("EquipRifle"), COREKIT_BASICUI_TEXT("BindRifle", "Equip rifle"), C::Binding, RebindText(), TEXT("IA_Primary")),
			Info(TEXT("EquipKatana"), COREKIT_BASICUI_TEXT("BindKatana", "Equip katana"), C::Binding, RebindText(), TEXT("IA_Secondary")),
			Info(TEXT("SwapWeapon"), COREKIT_BASICUI_TEXT("BindSwap", "Swap weapon"), C::Binding, RebindText(), TEXT("IA_WeaponWheel")),
			Info(TEXT("Attack"), COREKIT_BASICUI_TEXT("BindAttack", "Attack"), C::Binding, RebindText(), TEXT("IA_Attack")),
			Info(TEXT("Aim"), COREKIT_BASICUI_TEXT("BindAim", "Aim"), C::Binding, RebindText(), TEXT("IA_Aim")),
			Info(TEXT("Interact"), COREKIT_BASICUI_TEXT("BindInteract", "Interact"), C::Binding, RebindText(), TEXT("IA_Interact")),
			Info(TEXT("InspectBuild"), COREKIT_BASICUI_TEXT("BindInspect", "Inspect build"), C::Binding, RebindText(), TEXT("IA_InspectBuild")),
			Info(TEXT("Pause"), COREKIT_BASICUI_TEXT("BindPause", "Pause"), C::Locked,
				COREKIT_BASICUI_TEXT("PauseBody", "Pause stays on Escape (Menu on a gamepad) so you can always stop the run. It cannot be rebound.")),
		},
		{
			Info(TEXT("DisplayMode"), COREKIT_BASICUI_TEXT("DisplayMode", "Display mode"), C::Segmented,
				COREKIT_BASICUI_TEXT("DisplayModeBody", "Fullscreen gives the lowest input latency. Motion blur ships off: projectiles stay readable during dashes.")),
			Info(TEXT("Resolution"), COREKIT_BASICUI_TEXT("Resolution", "Resolution"), C::Arrows,
				COREKIT_BASICUI_TEXT("ResolutionBody", "Rendering size. Your monitor's native resolution keeps text, keycaps and the crosshair sharp. Changes wait for APPLY.")),
			Info(TEXT("FrameRate"), COREKIT_BASICUI_TEXT("FrameRate", "Frame rate limit"), C::Arrows,
				COREKIT_BASICUI_TEXT("FrameRateBody", "Caps the frame rate. Match your monitor's refresh rate for the steadiest aim; Unlimited lets the GPU run free.")),
			Info(TEXT("VSync"), COREKIT_BASICUI_TEXT("VSync", "V-Sync"), C::Segmented,
				COREKIT_BASICUI_TEXT("VSyncBody", "Removes screen tearing at the cost of some input latency. With a frame rate limit you can usually leave it off.")),
			Info(TEXT("MotionBlur"), COREKIT_BASICUI_TEXT("MotionBlur", "Motion blur"), C::Segmented,
				COREKIT_BASICUI_TEXT("MotionBlurBody", "Blurs fast turns and dashes. It ships off so drones and their shots stay readable at speed.")),
		},
		{
			Info(TEXT("MasterVolume"), COREKIT_BASICUI_TEXT("Master", "Master volume"), C::Slider,
				COREKIT_BASICUI_TEXT("MasterBody", "Scales every channel. Enemy charge-up and boss telegraph sounds are never ducked by music.")),
			Info(TEXT("MusicVolume"), COREKIT_BASICUI_TEXT("Music", "Music"), C::Slider,
				COREKIT_BASICUI_TEXT("MusicBody", "The soundtrack. The title screen plays the combat track behind a low-pass filter; the run opens it up.")),
			Info(TEXT("EffectsVolume"), COREKIT_BASICUI_TEXT("Effects", "Effects"), C::Slider,
				COREKIT_BASICUI_TEXT("EffectsBody", "Weapons, drones, pickups and interface sounds. Warnings stay audible at any music level.")),
			Info(TEXT("Heartbeat"), COREKIT_BASICUI_TEXT("Heartbeat", "Low-health heartbeat"), C::Segmented,
				COREKIT_BASICUI_TEXT("HeartbeatBody", "Plays a heartbeat loop while your health is under 30%. With it off, the red pulse around the screen still warns you.")),
		},
	};
	static const TArray<FCoreKitBasicUISettingsRowInfo> Empty;
	return Tab >= 0 && Tab < TabCount ? Tabs[Tab] : Empty;
}

TArray<FText> UCoreKitBasicUISettingsMenu::GetTabLabels()
{
	return { COREKIT_BASICUI_TEXT("TabGameplay", "GAMEPLAY"), COREKIT_BASICUI_TEXT("TabControls", "CONTROLS"), COREKIT_BASICUI_TEXT("TabVideo", "VIDEO"), COREKIT_BASICUI_TEXT("TabAudio", "AUDIO") };
}

// ---------------------------------------------------------------------------------------------------------------
// Screen

UCoreKitBasicUISettingsMenu::UCoreKitBasicUISettingsMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowReturn = true;
	bAllowSections = true;
}

bool UCoreKitBasicUISettingsMenu::HasUnappliedChanges() const
{
	return Draft && Draft->IsDirty();
}

int32 UCoreKitBasicUISettingsMenu::GetActiveTab() const
{
	return Tabs ? Tabs->GetActiveSection() : 0;
}

UInputMappingContext* UCoreKitBasicUISettingsMenu::GetMappingContext() const
{
	return MappingContext;
}

void UCoreKitBasicUISettingsMenu::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIFrontEndUI::MakeScreenRoot(WidgetTree);
	MappingContext = GetDefault<UCoreKitBasicUISettings>()->InputMapping.LoadSynchronous();

	TArray<FIntPoint> Supported;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Supported);
	// An unset resolution (first launch, editor) means the desktop one.
	FIntPoint Current = UCoreKitBasicUIUserSettings::Get()->Resolution;
	if ((Current.X <= 0 || Current.Y <= 0) && GEngine && GEngine->GetGameUserSettings())
	{
		Current = GEngine->GetGameUserSettings()->GetDesktopResolution();
		UCoreKitBasicUIUserSettings::Get()->Resolution = Current;
	}
	if (Current.X > 0 && Current.Y > 0)
	{
		Supported.AddUnique(Current);
	}
	Supported.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });
	for (const FIntPoint& Resolution : Supported)
	{
		Resolutions.AddUnique(Resolution);
	}

	// Header: SYSTEM CONFIG / SETTINGS; tabs with Q and E (LB / RB) on the right.
	UHorizontalBox* Header = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Header"));
	CoreKitBasicUIBuild::AddH(Header, CoreKitBasicUIFrontEndUI::MakeTitleBlock(WidgetTree, COREKIT_BASICUI_TEXT("Kicker", "SYSTEM CONFIG"), COREKIT_BASICUI_TEXT("Title", "Settings")), FMargin(0.f), VAlign_Bottom, HAlign_Left, 1.f);
	UHorizontalBox* TabRow = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("TabRow"));
	auto TabKey = [this](FKey Keyboard, FKey Pad)
	{
		USizeBox* Box = CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIFrontEndUI::MakeKeycap(WidgetTree, Keyboard, Pad), 0.f, 38.f);
		Box->SetMinDesiredWidth(38.f);
		return Box;
	};
	CoreKitBasicUIBuild::AddH(TabRow, TabKey(EKeys::Q, EKeys::Gamepad_LeftShoulder), FMargin(0.f, 0.f, 14.f, 0.f));
	Tabs = WidgetTree->ConstructWidget<UCoreKitSectionList>(UCoreKitSectionList::StaticClass(), TEXT("Tabs"));
	Tabs->OnStyleTab.AddUObject(this, &ThisClass::StyleTab);
	Tabs->SetSections(GetTabLabels());
	CoreKitBasicUIBuild::AddH(TabRow, Tabs);
	CoreKitBasicUIBuild::AddH(TabRow, TabKey(EKeys::E, EKeys::Gamepad_RightShoulder), FMargin(14.f, 0.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddH(Header, TabRow, FMargin(0.f), VAlign_Bottom);
	UCanvasPanelSlot* HeaderSlot = CoreKitBasicUIBuild::Place(Root, Header, FAnchors(0.f, 0.f, 1.f, 0.f), FVector2D::ZeroVector);
	HeaderSlot->SetOffsets(FMargin(120.f, 64.f, 120.f, 0.f));
	HeaderSlot->SetAutoSize(true);

	UCanvasPanelSlot* RuleSlot = CoreKitBasicUIBuild::Place(Root, CoreKitBasicUIFrontEndUI::MakeRule(WidgetTree, CoreKitBasicUIColor::Seam), FAnchors(0.f, 0.f, 1.f, 0.f), FVector2D::ZeroVector);
	RuleSlot->SetOffsets(FMargin(120.f, 200.f, 120.f, 1.f));

	// Body: rows (one scroll box per tab) and the SELECTED side panel (560 px).
	UHorizontalBox* Body = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Body"));
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	for (int32 Tab = 0; Tab < TabCount; ++Tab)
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), *FString::Printf(TEXT("Page%d"), Tab));
		Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
		UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree);
		Scroll->AddChild(Column);
		BuildRows(Tab, Column);
		Pages->AddChild(Scroll);
		Scrolls.Add(Scroll);
	}
	Content = Pages;
	CoreKitBasicUIBuild::AddH(Body, Pages, FMargin(0.f, 0.f, 56.f, 0.f), VAlign_Fill, HAlign_Fill, 1.f);

	UBorder* Panel = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(18, CoreKitBasicUIColor::Hull), FMargin(32.f, 30.f), TEXT("SidePanel"));
	UVerticalBox* PanelColumn = CoreKitBasicUIBuild::Column(WidgetTree);
	Panel->SetContent(PanelColumn);
	PanelKicker = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Selected", "SELECTED"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Magenta, 0.22f));
	CoreKitBasicUIBuild::AddV(PanelColumn, PanelKicker);
	CoreKitBasicUIBuild::FTextStyle TitleStyle = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(40.f), 700);
	TitleStyle.bUpper = true;
	PanelTitle = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), TitleStyle);
	PanelTitle->SetLineHeightPercentage(0.9f);
	CoreKitBasicUIBuild::AddV(PanelColumn, PanelTitle, FMargin(0.f, 14.f, 0.f, 0.f));
	PanelBody = CoreKitBasicUIFrontEndUI::MakeParagraph(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(22.f), 400, CoreKitBasicUIColor::TextDim));
	PanelBody->SetLineHeightPercentage(1.1f);
	CoreKitBasicUIBuild::AddV(PanelColumn, PanelBody, FMargin(0.f, 14.f, 0.f, 0.f));
	PanelHint = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.18f));
	CoreKitBasicUIBuild::AddV(PanelColumn, PanelHint, FMargin(0.f, 14.f, 0.f, 0.f));
	PanelStatus = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 700, CoreKitBasicUIColor::Mint, 0.14f));
	PanelStatus->SetAutoWrapText(true);
	CoreKitBasicUIBuild::AddV(PanelColumn, PanelStatus, FMargin(0.f, 10.f, 0.f, 0.f));
	CoreKitBasicUIBuild::AddV(PanelColumn, CoreKitBasicUIBuild::Box(WidgetTree, nullptr, 0.f, 0.f), FMargin(0.f), HAlign_Fill, VAlign_Fill, 1.f);

	// Live crosshair preview (Gameplay).
	UVerticalBox* Preview = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Preview"));
	CoreKitBasicUIBuild::AddV(Preview, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Preview", "PREVIEW"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Muted, 0.18f)), FMargin(0.f, 0.f, 0.f, 10.f));
	UOverlay* Frame = CoreKitBasicUIBuild::Overlay(WidgetTree);
	CoreKitBasicUIBuild::AddO(Frame, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot)));
	CoreKitBasicUIBuild::AddO(Frame, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Seam, 0.8f))), 0.f, 1.f), HAlign_Fill, VAlign_Center, FMargin(60.f, 0.f));
	UCanvasPanel* Marks = CoreKitBasicUIBuild::Canvas(WidgetTree);
	CoreKitBasicUIBuild::AddO(Frame, CoreKitBasicUIBuild::Box(WidgetTree, Marks, 60.f, 60.f), HAlign_Center, VAlign_Center);
	const FVector4f Parts[] = { FVector4f(28.f, 28.f, 4.f, 4.f), FVector4f(29.f, 6.f, 2.f, 12.f), FVector4f(29.f, 42.f, 2.f, 12.f), FVector4f(6.f, 29.f, 12.f, 2.f), FVector4f(42.f, 29.f, 12.f, 2.f) };
	for (const FVector4f& Part : Parts)
	{
		UImage* Mark = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::White));
		CoreKitBasicUIBuild::Place(Marks, Mark, FAnchors(0.f, 0.f), FVector2D(Part.X, Part.Y), FVector2D(Part.Z, Part.W));
		PreviewMarks.Add(Mark);
	}
	CoreKitBasicUIBuild::AddV(Preview, CoreKitBasicUIBuild::Box(WidgetTree, Frame, 0.f, 150.f));
	PreviewBlock = Preview;
	CoreKitBasicUIBuild::AddV(PanelColumn, Preview);

	PanelBox = CoreKitBasicUIBuild::Box(WidgetTree, Panel, 560.f, 540.f, TEXT("PanelBox"));
	CoreKitBasicUIBuild::AddH(Body, PanelBox, FMargin(0.f), VAlign_Top);
	UCanvasPanelSlot* BodySlot = CoreKitBasicUIBuild::Place(Root, Body, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector);
	BodySlot->SetOffsets(FMargin(120.f, 225.f, 120.f, 112.f));

	Tabs->SetSwitcher(Pages);
	Tabs->OnSectionChanged.AddUniqueDynamic(this, &ThisClass::HandleSectionChanged);
	SetSectionList(Tabs);

	// Footer: ESC BACK · T RESET TO DEFAULTS · F APPLY.
	UVerticalBox* Footer = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Footer"));
	CoreKitBasicUIBuild::AddV(Footer, CoreKitBasicUIFrontEndUI::MakeRule(WidgetTree, CoreKitBasicUIColor::Plate));
	UHorizontalBox* FooterRow = CoreKitBasicUIBuild::Row(WidgetTree);
	CoreKitBasicUIBuild::AddV(Footer, FooterRow, FMargin(0.f, 18.f, 0.f, 0.f));
	UCoreKitButton* BackButton = nullptr;
	UCoreKitButton* ResetButton = nullptr;
	UCoreKitButton* ApplyButton = nullptr;
	UTextBlock* ApplyText = nullptr;
	UCoreKitBasicUIDeviceKeycap* ApplyKey = nullptr;
	CoreKitBasicUIBuild::AddH(FooterRow, CoreKitBasicUIFrontEndUI::MakeKeyHint(WidgetTree, EKeys::Escape, EKeys::Gamepad_FaceButton_Right, COREKIT_BASICUI_TEXT("Back", "BACK"), CoreKitBasicUIColor::TextDim, CoreKitBasicUIColor::Steel, &BackButton));
	CoreKitBasicUIBuild::AddH(FooterRow, CoreKitBasicUIBuild::Box(WidgetTree, nullptr, 0.f, 0.f), FMargin(0.f), VAlign_Center, HAlign_Fill, 1.f);
	CoreKitBasicUIBuild::AddH(FooterRow, CoreKitBasicUIFrontEndUI::MakeKeyHint(WidgetTree, EKeys::T, EKeys::Gamepad_FaceButton_Top, COREKIT_BASICUI_TEXT("Reset", "RESET TO DEFAULTS"), CoreKitBasicUIColor::TextDim, CoreKitBasicUIColor::Steel, &ResetButton), FMargin(0.f, 0.f, 36.f, 0.f));
	CoreKitBasicUIBuild::AddH(FooterRow, CoreKitBasicUIFrontEndUI::MakeKeyHint(WidgetTree, EKeys::F, EKeys::Gamepad_FaceButton_Left, COREKIT_BASICUI_TEXT("Apply", "APPLY"), CoreKitBasicUIColor::Magenta, CoreKitBasicUIColor::Magenta, &ApplyButton, &ApplyText, &ApplyKey));
	ApplyLabel = ApplyText;
	ApplyKeycap = ApplyKey;
	BackButton->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleFooterBack);
	ResetButton->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleFooterReset);
	ApplyButton->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleFooterApply);
	UCanvasPanelSlot* FooterSlot = CoreKitBasicUIBuild::Place(Root, Footer, FAnchors(0.f, 1.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.f, 1.f));
	FooterSlot->SetOffsets(FMargin(120.f, -44.f, 120.f, 0.f));
	FooterSlot->SetAutoSize(true);

	// Gamepad stick navigation between rows (keys are handled in HandleMenuKey / NativeOnKeyDown).
	for (int32 Tab = 0; Tab < TabCount; ++Tab)
	{
		const TArray<int32> TabRows = RowsOfTab(Tab);
		for (int32 Position = 0; Position < TabRows.Num(); ++Position)
		{
			UCoreKitButton* Button = Rows[TabRows[Position]].Button;
			Button->SetNavigationRuleExplicit(EUINavigation::Up, Rows[TabRows[FMath::Max(Position - 1, 0)]].Button);
			Button->SetNavigationRuleExplicit(EUINavigation::Down, Rows[TabRows[FMath::Min(Position + 1, TabRows.Num() - 1)]].Button);
			FCustomWidgetNavigationDelegate Horizontal;
			Horizontal.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleHorizontalNavigation));
			Button->SetNavigationRuleCustom(EUINavigation::Left, Horizontal);
			Button->SetNavigationRuleCustom(EUINavigation::Right, Horizontal);
		}
	}
}

void UCoreKitBasicUISettingsMenu::BuildRows(int32 Tab, UVerticalBox* Column)
{
	const TArray<FCoreKitBasicUISettingsRowInfo>& Infos = GetRowInfos(Tab);
	Rows.Reserve(Rows.Num() + Infos.Num() + 32);
	for (int32 Index = 0; Index < Infos.Num(); ++Index)
	{
		const int32 RowIndex = Rows.AddDefaulted();
		FRow& Row = Rows[RowIndex];
		Row.Info = Infos[Index];
		Row.Tab = Tab;

		UOverlay* Stack = CoreKitBasicUIBuild::Overlay(WidgetTree);
		Row.Background = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Plate));
		Row.Background->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::AddO(Stack, Row.Background);
		Row.Inset = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta));
		CoreKitBasicUIBuild::AddO(Stack, CoreKitBasicUIBuild::Box(WidgetTree, Row.Inset, 6.f, 0.f), HAlign_Left, VAlign_Fill);
		Row.Button = CoreKitBasicUIFrontEndUI::MakeClearButton(WidgetTree, nullptr, true, *FString::Printf(TEXT("Row_%s"), *Row.Info.Id.ToString()));
		Row.Button->ButtonIndex = RowIndex;
		Row.Button->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleRowActivated);
		CoreKitBasicUIBuild::AddO(Stack, Row.Button);
		if (Index < Infos.Num() - 1)
		{
			UWidget* Rule = CoreKitBasicUIFrontEndUI::MakeRule(WidgetTree, CoreKitBasicUIColor::Plate);
			Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
			CoreKitBasicUIBuild::AddO(Stack, Rule, HAlign_Fill, VAlign_Bottom);
		}

		// Label · control (440 px) · value (90 px), gap 24, padding 0 28.
		UHorizontalBox* Line = CoreKitBasicUIBuild::Row(WidgetTree);
		Line->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Row.Label = CoreKitBasicUIBuild::Text(WidgetTree, Row.Info.Label, CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(24.f), 600));
		Row.Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::AddH(Line, Row.Label, FMargin(0.f, 0.f, 24.f, 0.f), VAlign_Center, HAlign_Left, 1.f);
		USizeBox* ControlBox = CoreKitBasicUIBuild::Box(WidgetTree, BuildControl(Row), 440.f, 0.f);
		ControlBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CoreKitBasicUIBuild::AddH(Line, ControlBox, FMargin(0.f, 0.f, 24.f, 0.f), VAlign_Center);
		CoreKitBasicUIBuild::FTextStyle ValueStyle = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(24.f), 600);
		ValueStyle.Justify = ETextJustify::Right;
		Row.Value = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), ValueStyle);
		Row.Value->SetVisibility(ESlateVisibility::HitTestInvisible);
		USizeBox* ValueBox = CoreKitBasicUIBuild::Box(WidgetTree, Row.Value, 90.f, 0.f);
		ValueBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::AddH(Line, ValueBox, FMargin(0.f), VAlign_Center, HAlign_Right);
		CoreKitBasicUIBuild::AddO(Stack, Line, HAlign_Fill, VAlign_Fill, FMargin(28.f, 0.f));

		Row.Root = CoreKitBasicUIBuild::Box(WidgetTree, Stack, 0.f, 70.f);
		if (Row.Info.Control == ECoreKitBasicUISettingsControl::Locked)
		{
			Row.Root->SetRenderOpacity(0.55f);
			Row.Button->SetIsEnabled(false);
		}
		CoreKitBasicUIBuild::AddV(Column, Row.Root);
	}
}

UWidget* UCoreKitBasicUISettingsMenu::BuildControl(FRow& Row)
{
	switch (Row.Info.Control)
	{
	case ECoreKitBasicUISettingsControl::Slider:
	{
		const FName Id = Row.Info.Id;
		Row.Min = Id == TEXT("FieldOfView") ? 80.f : Id == TEXT("HudScale") ? 75.f : 0.f;
		Row.Max = Id == TEXT("FieldOfView") ? 120.f : Id == TEXT("HudScale") ? 125.f : 100.f;
		Row.Step = Id == TEXT("MouseSensitivity") || Id == TEXT("FieldOfView") ? 1.f : 5.f;
		UOverlay* Track = CoreKitBasicUIBuild::Overlay(WidgetTree);
		Track->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UWidget* Groove = CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Seam)), 0.f, 8.f);
		Groove->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::AddO(Track, Groove, HAlign_Fill, VAlign_Center);
		Row.SliderFillImage = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::White));
		Row.SliderFill = CoreKitBasicUIBuild::Box(WidgetTree, Row.SliderFillImage, 1.f, 8.f);
		Row.SliderFill->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoreKitBasicUIBuild::AddO(Track, Row.SliderFill, HAlign_Left, VAlign_Center);
		Row.Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Row.Slider->IsFocusable = false;
		Row.Slider->SetMinValue(Row.Min);
		Row.Slider->SetMaxValue(Row.Max);
		Row.Slider->SetStepSize(Row.Step);
		Row.Slider->SetValue(GetSliderValue(Row));
		FSliderStyle Style;
		FSlateBrush None;
		None.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.SetNormalBarImage(None).SetHoveredBarImage(None).SetDisabledBarImage(None);
		FSlateBrush Thumb = UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta);
		Thumb.ImageSize = FVector2D(16.f, 30.f);
		Style.SetNormalThumbImage(Thumb).SetHoveredThumbImage(Thumb).SetDisabledThumbImage(Thumb);
		Style.SetBarThickness(8.f);
		Row.Slider->SetWidgetStyle(Style);
		Row.Slider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleSliderChanged);
		CoreKitBasicUIBuild::AddO(Track, Row.Slider, HAlign_Fill, VAlign_Center);
		USizeBox* TrackBox = CoreKitBasicUIBuild::Box(WidgetTree, Track, 0.f, 30.f);
		TrackBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return TrackBox;
	}
	case ECoreKitBasicUISettingsControl::Segmented:
	case ECoreKitBasicUISettingsControl::Arrows:
	{
		Row.Selector = WidgetTree->ConstructWidget<UCoreKitOptionSelector>(UCoreKitOptionSelector::StaticClass());
		Row.Selector->Mode = Row.Info.Control == ECoreKitBasicUISettingsControl::Arrows ? ECoreKitOptionSelectorMode::Arrows : ECoreKitOptionSelectorMode::Segmented;
		UCoreKitOptionSelector* Selector = Row.Selector;
		Selector->OnStyleOption.AddWeakLambda(this, [this, Selector](UCoreKitButton* Button, int32 Index, bool bSelected)
		{
			StyleSelector(Selector, Button, Index, bSelected);
		});
		Selector->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleOptionChanged);
		Selector->SetOptions(GetOptions(Row), GetOptionIndex(Row));
		Row.LastIndex = Selector->GetSelectedIndex();
		USizeBox* SelectorBox = CoreKitBasicUIBuild::Box(WidgetTree, Selector, 0.f, 40.f);
		SelectorBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return SelectorBox;
	}
	case ECoreKitBasicUISettingsControl::Swatches:
	{
		UHorizontalBox* Swatches = CoreKitBasicUIBuild::Row(WidgetTree);
		Swatches->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		const FLinearColor Colors[] = { CoreKitBasicUIColor::White, CoreKitBasicUIColor::Magenta, CoreKitBasicUIColor::Lime, CoreKitBasicUIColor::Mint };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Colors); ++Index)
		{
			// 40 x 40 swatch with a 3 px outline 3 px away when selected (52 x 52 footprint).
			UOverlay* Swatch = CoreKitBasicUIBuild::Overlay(WidgetTree);
			UImage* Ring = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::White));
			CoreKitBasicUIBuild::AddO(Swatch, Ring);
			CoreKitBasicUIBuild::AddO(Swatch, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Hull)), HAlign_Fill, VAlign_Fill, FMargin(3.f));
			CoreKitBasicUIBuild::AddO(Swatch, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(Colors[Index])), HAlign_Fill, VAlign_Fill, FMargin(6.f));
			UCoreKitButton* Button = CoreKitBasicUIFrontEndUI::MakeClearButton(WidgetTree, CoreKitBasicUIBuild::Box(WidgetTree, Swatch, 52.f, 52.f), true);
			Button->ButtonIndex = Index;
			Button->OnActivated.AddUniqueDynamic(this, &ThisClass::HandleSwatchActivated);
			CoreKitBasicUIBuild::AddH(Swatches, Button, FMargin(Index == 0 ? -6.f : 0.f, 0.f, 0.f, 0.f));
			Row.Swatches.Add(Button);
			Row.SwatchRings.Add(Ring);
		}
		return Swatches;
	}
	case ECoreKitBasicUISettingsControl::Binding:
	case ECoreKitBasicUISettingsControl::Locked:
	{
		UHorizontalBox* Binding = CoreKitBasicUIBuild::Row(WidgetTree);
		Binding->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row.Keycap = Row.Info.Control == ECoreKitBasicUISettingsControl::Locked
			? CoreKitBasicUIFrontEndUI::MakeKeycap(WidgetTree, EKeys::Escape, EKeys::Gamepad_Special_Right)
			: WidgetTree->ConstructWidget<UCoreKitBasicUIKeycap>(UCoreKitBasicUIKeycap::StaticClass());
		if (Row.Info.Control == ECoreKitBasicUISettingsControl::Binding && !Row.Info.Action.IsNone())
		{
			Row.InputAction = GetDefault<UCoreKitBasicUISettings>()->FindAction(Row.Info.Action);
			if (Row.InputAction)
			{
				Actions.AddUnique(Row.InputAction);
			}
			const UInputAction* RowAction = Row.InputAction;
			const bool bMapped = RowAction && MappingContext && MappingContext->GetMappings().ContainsByPredicate(
				[RowAction](const FEnhancedActionKeyMapping& Item) { return Item.Action == RowAction; });
			if (bMapped)
			{
				Row.Keycap->SetAction(Row.InputAction);
			}
			else
			{
				Row.Keycap->SetFixedText(COREKIT_BASICUI_TEXT("Unbound", "—"));
			}
		}
		USizeBox* KeyBox = CoreKitBasicUIBuild::Box(WidgetTree, Row.Keycap, 0.f, 38.f);
		KeyBox->SetMinDesiredWidth(38.f);
		CoreKitBasicUIBuild::AddH(Binding, KeyBox);
		Row.Listening = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("PressKey", "PRESS A KEY…"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), 500, CoreKitBasicUIColor::Muted, 0.08f));
		Row.Listening->SetVisibility(ESlateVisibility::Collapsed);
		CoreKitBasicUIBuild::AddH(Binding, Row.Listening, FMargin(8.f, 0.f, 0.f, 0.f), VAlign_Center);
		return Binding;
	}
	}
	return CoreKitBasicUIBuild::Box(WidgetTree, nullptr, 0.f, 0.f);
}

void UCoreKitBasicUISettingsMenu::StyleSelector(UCoreKitOptionSelector* Selector, UCoreKitButton* Button, int32 Index, bool bSelected)
{
	if (!Selector || !Button || !Selector->WidgetTree)
	{
		return;
	}
	UWidgetTree* Tree = Selector->WidgetTree;
	// First call: wrap the generated label in an overlay with the 3 px magenta underline.
	UOverlay* Stack = Cast<UOverlay>(Button->GetContent());
	UTextBlock* Label = Cast<UTextBlock>(Button->GetContent());
	if (Label)
	{
		Stack = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Button->SetContent(Stack);
		Stack->AddChildToOverlay(Label);
		UImage* Underline = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Underline->SetBrush(UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta));
		USizeBox* LineBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LineBox->SetHeightOverride(3.f);
		LineBox->AddChild(Underline);
		UOverlaySlot* LineSlot = Stack->AddChildToOverlay(LineBox);
		LineSlot->SetHorizontalAlignment(HAlign_Fill);
		LineSlot->SetVerticalAlignment(VAlign_Bottom);
		if (UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Label->Slot))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(14.f, 0.f));
		}
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Stack->Slot))
		{
			ButtonSlot->SetPadding(FMargin(0.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Button->Slot))
		{
			const bool bArrow = Index < 0;
			BoxSlot->SetPadding(bArrow ? FMargin(0.f) : FMargin(Index > 0 ? 2.f : 0.f, 0.f, Index < Selector->GetOptionCount() - 1 ? 2.f : 0.f, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	if (!Stack || Stack->GetChildrenCount() < 2)
	{
		return;
	}
	Label = Cast<UTextBlock>(Stack->GetChildAt(0));
	UWidget* Underline = Stack->GetChildAt(1);
	if (Index < 0)
	{
		// Arrow buttons of the ◀ value ▶ selector.
		Button->SetBaseStyle(UCoreKitBasicUIStyle::ButtonStyle(8, CoreKitBasicUIColor::Plate, CoreKitBasicUIColor::Seam, CoreKitBasicUIColor::Seam));
		CoreKitBasicUIBuild::ApplyTextStyle(Label, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(16.f), 700, CoreKitBasicUIColor::White, 0.f));
		Underline->SetVisibility(ESlateVisibility::Collapsed);
		if (UTextBlock* Value = Tree->FindWidget<UTextBlock>(TEXT("ValueLabel")))
		{
			CoreKitBasicUIBuild::FTextStyle ValueStyle = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(24.f), 600);
			ValueStyle.Justify = ETextJustify::Center;
			CoreKitBasicUIBuild::ApplyTextStyle(Value, ValueStyle);
		}
		return;
	}
	// Segmented: Plate / Steel; selected White / Soot bold with the magenta underline.
	const FLinearColor Fill = bSelected ? CoreKitBasicUIColor::White : CoreKitBasicUIColor::Plate;
	Button->SetBaseStyle(UCoreKitBasicUIStyle::ButtonStyle(8, Fill, bSelected ? CoreKitBasicUIColor::White : CoreKitBasicUIColor::Seam, Fill));
	CoreKitBasicUIBuild::ApplyTextStyle(Label, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), bSelected ? 700 : 500, bSelected ? CoreKitBasicUIColor::Soot : CoreKitBasicUIColor::Steel, 0.12f));
	Underline->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UCoreKitBasicUISettingsMenu::StyleTab(UCoreKitButton* Button, int32 Index, bool bActive)
{
	if (!Button || !Tabs || !Tabs->WidgetTree)
	{
		return;
	}
	UWidgetTree* Tree = Tabs->WidgetTree;
	UOverlay* Stack = Cast<UOverlay>(Button->GetContent());
	if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
	{
		Stack = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Button->SetContent(Stack);
		Stack->AddChildToOverlay(Label);
		UImage* Underline = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Underline->SetBrush(UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta));
		USizeBox* LineBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LineBox->SetHeightOverride(4.f);
		LineBox->AddChild(Underline);
		UOverlaySlot* LineSlot = Stack->AddChildToOverlay(LineBox);
		LineSlot->SetHorizontalAlignment(HAlign_Fill);
		LineSlot->SetVerticalAlignment(VAlign_Bottom);
		if (UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Label->Slot))
		{
			LabelSlot->SetPadding(FMargin(30.f, 12.f));
		}
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Stack->Slot))
		{
			ButtonSlot->SetPadding(FMargin(0.f));
		}
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Button->Slot))
		{
			BoxSlot->SetPadding(FMargin(Index > 0 ? 3.f : 0.f, 0.f, 3.f, 0.f));
		}
	}
	if (!Stack || Stack->GetChildrenCount() < 2)
	{
		return;
	}
	UTextBlock* Label = Cast<UTextBlock>(Stack->GetChildAt(0));
	CoreKitBasicUIBuild::ApplyTextStyle(Label, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(26.f), 700, bActive ? CoreKitBasicUIColor::Soot : CoreKitBasicUIColor::Steel));
	Stack->GetChildAt(1)->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Button->SetBaseStyle(bActive ? UCoreKitBasicUIStyle::ButtonStyle(14, CoreKitBasicUIColor::White, CoreKitBasicUIColor::White, CoreKitBasicUIColor::White)
		: UCoreKitBasicUIStyle::ButtonStyle(14, CoreKitBasicUIColor::Transparent, CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Plate, 0.8f), CoreKitBasicUIColor::Plate));
}

// ---------------------------------------------------------------------------------------------------------------
// Values

float UCoreKitBasicUISettingsMenu::GetSliderValue(const FRow& Row) const
{
	const UCoreKitBasicUIUserSettings* S = UCoreKitBasicUIUserSettings::Get();
	const FName Id = Row.Info.Id;
	if (Id == TEXT("MouseSensitivity")) return S->MouseSensitivity;
	if (Id == TEXT("FieldOfView")) return S->FieldOfView;
	if (Id == TEXT("ScreenShake")) return S->ScreenShake;
	if (Id == TEXT("HudScale")) return S->HudScale;
	if (Id == TEXT("MasterVolume")) return S->MasterVolume;
	if (Id == TEXT("MusicVolume")) return S->MusicVolume;
	if (Id == TEXT("EffectsVolume")) return S->EffectsVolume;
	return 0.f;
}

void UCoreKitBasicUISettingsMenu::SetSliderValue(FRow& Row, float Value)
{
	UCoreKitBasicUIUserSettings* S = UCoreKitBasicUIUserSettings::Get();
	const float Clamped = FMath::Clamp(FMath::RoundToFloat(Value), Row.Min, Row.Max);
	const FName Id = Row.Info.Id;
	if (Id == TEXT("MouseSensitivity")) S->MouseSensitivity = Clamped;
	else if (Id == TEXT("FieldOfView")) S->FieldOfView = Clamped;
	else if (Id == TEXT("ScreenShake")) S->ScreenShake = Clamped;
	else if (Id == TEXT("HudScale")) S->HudScale = Clamped;
	else if (Id == TEXT("MasterVolume")) S->MasterVolume = Clamped;
	else if (Id == TEXT("MusicVolume")) S->MusicVolume = Clamped;
	else if (Id == TEXT("EffectsVolume")) S->EffectsVolume = Clamped;
	RefreshRow(Row);
}

TArray<FText> UCoreKitBasicUISettingsMenu::GetOptions(const FRow& Row) const
{
	const FName Id = Row.Info.Id;
	const FText Off = COREKIT_BASICUI_TEXT("Off", "OFF");
	const FText On = COREKIT_BASICUI_TEXT("On", "ON");
	if (Id == TEXT("DashNoInput")) return { COREKIT_BASICUI_TEXT("Forward", "FORWARD"), COREKIT_BASICUI_TEXT("Backward", "BACKWARD") };
	if (Id == TEXT("GamepadGlyphs")) return { COREKIT_BASICUI_TEXT("Auto", "AUTO"), COREKIT_BASICUI_TEXT("Xbox", "XBOX"), COREKIT_BASICUI_TEXT("PlayStation", "PLAYSTATION") };
	if (Id == TEXT("DisplayMode")) return { COREKIT_BASICUI_TEXT("Fullscreen", "FULLSCREEN"), COREKIT_BASICUI_TEXT("Borderless", "BORDERLESS"), COREKIT_BASICUI_TEXT("Windowed", "WINDOWED") };
	if (Id == TEXT("Resolution"))
	{
		TArray<FText> Options;
		for (const FIntPoint& Resolution : Resolutions)
		{
			Options.Add(FText::Format(COREKIT_BASICUI_TEXT("ResolutionValue", "{0} × {1}"), FText::AsNumber(Resolution.X, &FNumberFormattingOptions::DefaultNoGrouping()),
				FText::AsNumber(Resolution.Y, &FNumberFormattingOptions::DefaultNoGrouping())));
		}
		return Options;
	}
	if (Id == TEXT("FrameRate"))
	{
		TArray<FText> Options;
		for (int32 Rate : FrameRates)
		{
			Options.Add(Rate > 0 ? FText::AsNumber(Rate) : COREKIT_BASICUI_TEXT("Unlimited", "UNLIMITED"));
		}
		return Options;
	}
	return { Off, On };
}

int32 UCoreKitBasicUISettingsMenu::GetOptionIndex(const FRow& Row) const
{
	const UCoreKitBasicUIUserSettings* S = UCoreKitBasicUIUserSettings::Get();
	const FName Id = Row.Info.Id;
	if (Id == TEXT("InvertLookY")) return S->bInvertLookY ? 1 : 0;
	if (Id == TEXT("DashNoInput")) return S->bDashBackwardWithoutInput ? 1 : 0;
	if (Id == TEXT("DamageNumbers")) return S->bDamageNumbers ? 1 : 0;
	if (Id == TEXT("Crosshair")) return FMath::Clamp(S->CrosshairColor, 0, 3);
	if (Id == TEXT("ReducedMotion")) return S->bReducedMotion ? 1 : 0;
	if (Id == TEXT("GamepadGlyphs")) return FMath::Clamp(S->GamepadGlyphs, 0, 2);
	if (Id == TEXT("DisplayMode")) return FMath::Clamp(S->DisplayMode, 0, 2);
	if (Id == TEXT("Resolution")) return FMath::Max(Resolutions.IndexOfByKey(S->Resolution), 0);
	if (Id == TEXT("FrameRate"))
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(FrameRates); ++Index)
		{
			if (FrameRates[Index] == S->FrameRateLimit)
			{
				return Index;
			}
		}
		return 3;
	}
	if (Id == TEXT("VSync")) return S->bVSync ? 1 : 0;
	if (Id == TEXT("MotionBlur")) return S->bMotionBlur ? 1 : 0;
	if (Id == TEXT("Heartbeat")) return S->bLowHealthHeartbeat ? 1 : 0;
	return 0;
}

void UCoreKitBasicUISettingsMenu::SetOptionIndex(FRow& Row, int32 Index)
{
	UCoreKitBasicUIUserSettings* S = UCoreKitBasicUIUserSettings::Get();
	const FName Id = Row.Info.Id;
	const bool bOn = Index == 1;
	if (Id == TEXT("InvertLookY")) S->bInvertLookY = bOn;
	else if (Id == TEXT("DashNoInput")) S->bDashBackwardWithoutInput = bOn;
	else if (Id == TEXT("DamageNumbers")) S->bDamageNumbers = bOn;
	else if (Id == TEXT("Crosshair")) S->CrosshairColor = FMath::Clamp(Index, 0, 3);
	else if (Id == TEXT("ReducedMotion")) S->bReducedMotion = bOn;
	else if (Id == TEXT("GamepadGlyphs")) S->GamepadGlyphs = FMath::Clamp(Index, 0, 2);
	else if (Id == TEXT("DisplayMode")) S->DisplayMode = FMath::Clamp(Index, 0, 2);
	else if (Id == TEXT("Resolution") && Resolutions.IsValidIndex(Index)) S->Resolution = Resolutions[Index];
	else if (Id == TEXT("FrameRate")) S->FrameRateLimit = FrameRates[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(FrameRates) - 1)];
	else if (Id == TEXT("VSync")) S->bVSync = bOn;
	else if (Id == TEXT("MotionBlur")) S->bMotionBlur = bOn;
	else if (Id == TEXT("Heartbeat")) S->bLowHealthHeartbeat = bOn;
	RefreshRow(Row);
}

FText UCoreKitBasicUISettingsMenu::FormatValue(const FRow& Row) const
{
	if (Row.Info.Control == ECoreKitBasicUISettingsControl::Locked)
	{
		return COREKIT_BASICUI_TEXT("Locked", "LOCKED");
	}
	if (Row.Info.Control != ECoreKitBasicUISettingsControl::Slider)
	{
		return FText::GetEmpty();
	}
	const float Value = GetSliderValue(Row);
	const FName Id = Row.Info.Id;
	if (Id == TEXT("MouseSensitivity"))
	{
		FNumberFormattingOptions Two;
		Two.SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);
		return FText::AsNumber(Value / 100.f, &Two);
	}
	if (Id == TEXT("FieldOfView"))
	{
		return FText::Format(COREKIT_BASICUI_TEXT("Degrees", "{0}°"), FText::AsNumber(FMath::RoundToInt(Value)));
	}
	if (Id == TEXT("ScreenShake") || Id == TEXT("HudScale"))
	{
		return FText::Format(COREKIT_BASICUI_TEXT("Percent", "{0}%"), FText::AsNumber(FMath::RoundToInt(Value)));
	}
	return FText::AsNumber(FMath::RoundToInt(Value));
}

void UCoreKitBasicUISettingsMenu::RefreshRow(FRow& Row)
{
	if (Row.Slider)
	{
		const float Value = GetSliderValue(Row);
		Row.LastSlider = Value;
		if (!FMath::IsNearlyEqual(Row.Slider->GetValue(), Value))
		{
			Row.Slider->SetValue(Value);
		}
		const float Normalized = Row.Max > Row.Min ? (Value - Row.Min) / (Row.Max - Row.Min) : 0.f;
		Row.SliderFill->SetWidthOverride(FMath::Max(8.f + Normalized * (440.f - 16.f), 1.f));
	}
	if (Row.Selector)
	{
		const int32 Index = GetOptionIndex(Row);
		Row.LastIndex = Index;
		if (Row.Selector->GetSelectedIndex() != Index)
		{
			Row.Selector->SetSelectedIndex(Index, false);
		}
	}
	if (Row.Swatches.Num() > 0)
	{
		const int32 Index = GetOptionIndex(Row);
		Row.LastIndex = Index;
		for (int32 Swatch = 0; Swatch < Row.SwatchRings.Num(); ++Swatch)
		{
			Row.SwatchRings[Swatch]->SetVisibility(Swatch == Index ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}
	}
	if (Row.Value)
	{
		Row.Value->SetText(FormatValue(Row));
	}
}

void UCoreKitBasicUISettingsMenu::RefreshAllRows()
{
	for (FRow& Row : Rows)
	{
		RefreshRow(Row);
	}
}

void UCoreKitBasicUISettingsMenu::RefreshVisuals()
{
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		FRow& Row = Rows[Index];
		const bool bFocused = Index == FocusedRow;
		const bool bListening = Index == ListeningRow;
		Row.Background->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		Row.Inset->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (Row.SliderFillImage)
		{
			Row.SliderFillImage->SetColorAndOpacity(bFocused ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White);
		}
		if (Row.Value)
		{
			const bool bAccent = bFocused && Row.Info.Control == ECoreKitBasicUISettingsControl::Slider;
			Row.Value->SetColorAndOpacity(FSlateColor(Row.Info.Control == ECoreKitBasicUISettingsControl::Locked ? CoreKitBasicUIColor::Steel : bAccent ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White));
			if (Row.Info.Control == ECoreKitBasicUISettingsControl::Locked)
			{
				CoreKitBasicUIBuild::ApplyTextStyle(Row.Value, [] { CoreKitBasicUIBuild::FTextStyle S = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 500, CoreKitBasicUIColor::Steel, 0.14f); S.Justify = ETextJustify::Right; return S; }());
			}
		}
		if (Row.Keycap && Row.Info.Control == ECoreKitBasicUISettingsControl::Binding)
		{
			Row.Keycap->SetAccent(bListening ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::Steel);
			Row.Listening->SetVisibility(bListening ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	const bool bDirty = HasUnappliedChanges();
	if (ApplyLabel)
	{
		ApplyLabel->SetColorAndOpacity(FSlateColor(bDirty ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::Muted));
	}
	if (ApplyKeycap)
	{
		ApplyKeycap->SetAccent(bDirty ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::Steel);
	}
}

void UCoreKitBasicUISettingsMenu::RefreshSidePanel()
{
	const int32 Tab = GetActiveTab();
	const bool bControls = Tab == Controls;
	if (PanelBox)
	{
		const float Heights[] = { 540.f, 470.f, 400.f, 330.f };
		PanelBox->SetHeightOverride(Heights[FMath::Clamp(Tab, 0, 3)]);
	}
	const FRow* Row = Rows.IsValidIndex(FocusedRow) && Rows[FocusedRow].Tab == Tab ? &Rows[FocusedRow] : nullptr;
	PanelKicker->SetText(bControls ? COREKIT_BASICUI_TEXT("Rebinding", "REBINDING") : COREKIT_BASICUI_TEXT("Selected", "SELECTED"));
	PanelTitle->SetText(Row ? Row->Info.Label.ToUpper() : FText::GetEmpty());
	PanelBody->SetText(Row ? Row->Info.Description : FText::GetEmpty());
	const bool bBindingRow = Row && Row->Info.Control == ECoreKitBasicUISettingsControl::Binding;
	PanelHint->SetText(bBindingRow ? COREKIT_BASICUI_TEXT("RebindHint", "ENTER REBIND · BKSP RESET ROW · T RESET ALL") : FText::GetEmpty());
	PanelHint->SetVisibility(bBindingRow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	PreviewBlock->SetVisibility(Tab == Gameplay ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	const FLinearColor Crosshair = UCoreKitBasicUIUserSettings::Get()->GetCrosshairLinearColor();
	for (UImage* Mark : PreviewMarks)
	{
		Mark->SetColorAndOpacity(Crosshair);
	}
}

void UCoreKitBasicUISettingsMenu::OnValueEdited()
{
	if (Draft)
	{
		Draft->Preview(this);
	}
	RefreshVisuals();
	RefreshSidePanel();
}

// ---------------------------------------------------------------------------------------------------------------
// Lifetime and focus

void UCoreKitBasicUISettingsMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (!Draft)
	{
		Draft = NewObject<UCoreKitBasicUISettingsDraft>(this);
		Draft->Begin(UCoreKitBasicUIUserSettings::Get());
	}
	// Rebinding needs IMC_Combat registered with the player's Enhanced Input user settings (also outside a run).
	ULocalPlayer* Player = GetOwningLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Input = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (UEnhancedInputUserSettings* Settings = Input ? Input->GetUserSettings() : nullptr; Settings && MappingContext)
	{
		Settings->RegisterInputMappingContext(MappingContext);
	}
	RefreshAllRows();
	RefreshVisuals();
	RefreshSidePanel();
}

void UCoreKitBasicUISettingsMenu::NativeDestruct()
{
	StopListening();
	// Removed without Back (travel, pause closing): unapplied edits do not survive.
	if (!bClosing && Draft && Draft->IsDirty())
	{
		Draft->Discard(this);
	}
	Super::NativeDestruct();
}

void UCoreKitBasicUISettingsMenu::NativeOnMenuActivated()
{
	Super::NativeOnMenuActivated();
	// Returning from a prompt: CoreKitNavigation restores the first focus target after this call, so re-select the
	// row we were on in the next tick.
	bRestoreFocus = Rows.IsValidIndex(FocusedRow);
	if (!bRestoreFocus)
	{
		ContentIn.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle);
	}
}

UWidget* UCoreKitBasicUISettingsMenu::GetInitialFocus_Implementation() const
{
	const TArray<int32> First = RowsOfTab(GetActiveTab());
	return First.Num() > 0 ? Rows[First[0]].Button : nullptr;
}

TArray<int32> UCoreKitBasicUISettingsMenu::RowsOfTab(int32 Tab) const
{
	TArray<int32> Result;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index].Tab == Tab && Rows[Index].Info.Control != ECoreKitBasicUISettingsControl::Locked)
		{
			Result.Add(Index);
		}
	}
	return Result;
}

int32 UCoreKitBasicUISettingsMenu::FindFocusedRow() const
{
	const int32 Tab = GetActiveTab();
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const FRow& Row = Rows[Index];
		if (Row.Tab == Tab && (Row.Button->HasKeyboardFocus() || Row.Root->HasFocusedDescendants()))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UCoreKitBasicUISettingsMenu::FocusRow(int32 RowIndex)
{
	if (!Rows.IsValidIndex(RowIndex))
	{
		return;
	}
	FRow& Row = Rows[RowIndex];
	Row.Button->Select();
	FocusedRow = RowIndex;
	if (Scrolls.IsValidIndex(Row.Tab))
	{
		Scrolls[Row.Tab]->ScrollWidgetIntoView(Row.Root, false, EDescendantScrollDestination::IntoView);
	}
	RefreshVisuals();
	RefreshSidePanel();
}

void UCoreKitBasicUISettingsMenu::MoveFocus(int32 Delta)
{
	const TArray<int32> TabRows = RowsOfTab(GetActiveTab());
	if (TabRows.IsEmpty())
	{
		return;
	}
	const int32 Position = TabRows.IndexOfByKey(FocusedRow);
	// The row button plays the navigation sound when it takes focus.
	const int32 Next = Position == INDEX_NONE ? 0 : FMath::Clamp(Position + Delta, 0, TabRows.Num() - 1);
	FocusRow(TabRows[Next]);
}

void UCoreKitBasicUISettingsMenu::StepRow(int32 RowIndex, int32 Delta)
{
	if (!Rows.IsValidIndex(RowIndex) || Delta == 0 || IsListening())
	{
		return;
	}
	FRow& Row = Rows[RowIndex];
	switch (Row.Info.Control)
	{
	case ECoreKitBasicUISettingsControl::Slider:
		SetSliderValue(Row, GetSliderValue(Row) + Row.Step * Delta);
		break;
	case ECoreKitBasicUISettingsControl::Segmented:
	case ECoreKitBasicUISettingsControl::Arrows:
	{
		const int32 Before = GetOptionIndex(Row);
		SetOptionIndex(Row, FMath::Clamp(Before + Delta, 0, GetOptions(Row).Num() - 1));
		if (GetOptionIndex(Row) == Before)
		{
			return;
		}
		break;
	}
	case ECoreKitBasicUISettingsControl::Swatches:
		SetOptionIndex(Row, FMath::Clamp(GetOptionIndex(Row) + Delta, 0, 3));
		break;
	default:
		return;
	}
	PlayCue(TEXT("UI.Select"));
	OnValueEdited();
}

void UCoreKitBasicUISettingsMenu::ActivateRow(int32 RowIndex)
{
	if (!Rows.IsValidIndex(RowIndex))
	{
		return;
	}
	FRow& Row = Rows[RowIndex];
	switch (Row.Info.Control)
	{
	case ECoreKitBasicUISettingsControl::Binding:
		BeginListening(RowIndex);
		break;
	case ECoreKitBasicUISettingsControl::Segmented:
	case ECoreKitBasicUISettingsControl::Arrows:
	{
		// Enter / A cycles the value (toggles OFF / ON).
		const int32 Count = GetOptions(Row).Num();
		SetOptionIndex(Row, Count > 0 ? (GetOptionIndex(Row) + 1) % Count : 0);
		OnValueEdited();
		break;
	}
	case ECoreKitBasicUISettingsControl::Swatches:
		SetOptionIndex(Row, (GetOptionIndex(Row) + 1) % 4);
		OnValueEdited();
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Input

bool UCoreKitBasicUISettingsMenu::HandleMenuKey_Implementation(FKey Key)
{
	if (IsListening())
	{
		return true;
	}
	if (IsKey(Key, { EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up }))
	{
		MoveFocus(-1);
		return true;
	}
	if (IsKey(Key, { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down }))
	{
		MoveFocus(1);
		return true;
	}
	if (IsKey(Key, { EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left }))
	{
		StepRow(FocusedRow, -1);
		return true;
	}
	if (IsKey(Key, { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right }))
	{
		StepRow(FocusedRow, 1);
		return true;
	}
	if (IsKey(Key, { EKeys::F, EKeys::Gamepad_FaceButton_Left }))
	{
		ApplyChanges();
		return true;
	}
	if (IsKey(Key, { EKeys::T, EKeys::Gamepad_FaceButton_Top }))
	{
		RequestReset();
		return true;
	}
	if (IsKey(Key, { EKeys::BackSpace, EKeys::Delete, EKeys::Gamepad_Special_Left }) && Rows.IsValidIndex(FocusedRow) && Rows[FocusedRow].Info.Control == ECoreKitBasicUISettingsControl::Binding)
	{
		ResetRowBinding(FocusedRow);
		return true;
	}
	return false;
}

FReply UCoreKitBasicUISettingsMenu::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	// Held keys repeat here (CoreKitNavigation forwards only the first press to HandleMenuKey).
	if (Event.IsRepeat() && !IsListening())
	{
		const FKey Key = Event.GetKey();
		if (IsKey(Key, { EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up })) { MoveFocus(-1); return FReply::Handled(); }
		if (IsKey(Key, { EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down })) { MoveFocus(1); return FReply::Handled(); }
		if (IsKey(Key, { EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left })) { StepRow(FocusedRow, -1); return FReply::Handled(); }
		if (IsKey(Key, { EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right })) { StepRow(FocusedRow, 1); return FReply::Handled(); }
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

UWidget* UCoreKitBasicUISettingsMenu::HandleHorizontalNavigation(EUINavigation Direction)
{
	// Stick left/right on a row changes its value; focus stays.
	StepRow(FocusedRow, Direction == EUINavigation::Left ? -1 : 1);
	return nullptr;
}

bool UCoreKitBasicUISettingsMenu::HandleReturn_Implementation()
{
	if (IsListening())
	{
		return true;
	}
	RequestBack();
	return true;
}

void UCoreKitBasicUISettingsMenu::HandleSliderChanged(float)
{
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		FRow& Row = Rows[Index];
		if (!Row.Slider)
		{
			continue;
		}
		const float Current = GetSliderValue(Row);
		if (FMath::IsNearlyEqual(Row.Slider->GetValue(), Current, 0.01f))
		{
			continue;
		}
		// Only a drag or click (the slider is not focusable) is an edit; range clamps and SetValue are not.
		if (!Row.Slider->HasMouseCapture())
		{
			Row.Slider->SetValue(Current);
			continue;
		}
		SetSliderValue(Row, Row.Slider->GetValue());
		FocusedRow = Index;
		if (!FMath::IsNearlyEqual(Current, GetSliderValue(Row)))
		{
			OnValueEdited();
		}
	}
}

void UCoreKitBasicUISettingsMenu::HandleOptionChanged(int32)
{
	for (FRow& Row : Rows)
	{
		if (Row.Selector && Row.Selector->GetSelectedIndex() != Row.LastIndex)
		{
			SetOptionIndex(Row, Row.Selector->GetSelectedIndex());
			PlayCue(TEXT("UI.Select"));
			OnValueEdited();
		}
	}
}

void UCoreKitBasicUISettingsMenu::HandleSwatchActivated(UCoreKitButton* Button)
{
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index].Swatches.Contains(Button))
		{
			SetOptionIndex(Rows[Index], Button->ButtonIndex);
			FocusedRow = Index;
			OnValueEdited();
			return;
		}
	}
}

void UCoreKitBasicUISettingsMenu::HandleRowActivated(UCoreKitButton* Button)
{
	if (Button && !IsListening())
	{
		FocusedRow = Button->ButtonIndex;
		ActivateRow(Button->ButtonIndex);
	}
}

void UCoreKitBasicUISettingsMenu::HandleSectionChanged(int32 Section)
{
	StopListening();
	ContentIn.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle);
	PlayCue(TEXT("UI.Select"));
	const TArray<int32> TabRows = RowsOfTab(Section);
	if (TabRows.Num() > 0)
	{
		FocusRow(TabRows[0]);
	}
	RefreshSidePanel();
}

void UCoreKitBasicUISettingsMenu::HandleFooterBack(UCoreKitButton*) { RequestBack(); }
void UCoreKitBasicUISettingsMenu::HandleFooterReset(UCoreKitButton*) { RequestReset(); }
void UCoreKitBasicUISettingsMenu::HandleFooterApply(UCoreKitButton*) { ApplyChanges(); }

// ---------------------------------------------------------------------------------------------------------------
// Commands

void UCoreKitBasicUISettingsMenu::ApplyChanges()
{
	if (!Draft || IsListening())
	{
		return;
	}
	const bool bDirty = Draft->IsDirty();
	Draft->Commit(this);
	PlayCue(TEXT("UI.Confirm"));
	SetStatus(bDirty ? COREKIT_BASICUI_TEXT("Applied", "SETTINGS APPLIED") : COREKIT_BASICUI_TEXT("NothingToApply", "NOTHING TO APPLY"), bDirty ? CoreKitBasicUIColor::Mint : CoreKitBasicUIColor::Muted);
	RefreshVisuals();
}

void UCoreKitBasicUISettingsMenu::RequestReset()
{
	if (IsListening())
	{
		return;
	}
	TWeakObjectPtr<UCoreKitBasicUISettingsMenu> WeakThis(this);
	CoreKitBasicUIFrontEndUI::Confirm(this, ECoreKitBasicUIPromptPreset::ResetDefaults,
		COREKIT_BASICUI_TEXT("ResetBody", "Every setting on every tab goes back to its default and every key binding is restored now. APPLY keeps the new values."),
		[WeakThis](bool bConfirmed)
		{
			UCoreKitBasicUISettingsMenu* Menu = WeakThis.Get();
			if (!bConfirmed || !Menu)
			{
				return;
			}
			UCoreKitBasicUIUserSettings::Get()->ResetToDefaults();
			Menu->ResetAllBindings();
			Menu->RefreshAllRows();
			Menu->OnValueEdited();
			Menu->SetStatus(COREKIT_BASICUI_TEXT("ResetDone", "DEFAULTS RESTORED · APPLY TO KEEP"), CoreKitBasicUIColor::Mint);
		});
}

void UCoreKitBasicUISettingsMenu::RequestBack()
{
	if (bClosing)
	{
		return;
	}
	if (!HasUnappliedChanges())
	{
		bClosing = true;
		CoreKitBasicUIFrontEnd::CloseScreen(this);
		return;
	}
	TWeakObjectPtr<UCoreKitBasicUISettingsMenu> WeakThis(this);
	CoreKitBasicUIFrontEndUI::Confirm(this, ECoreKitBasicUIPromptPreset::DiscardChanges,
		COREKIT_BASICUI_TEXT("DiscardBody", "Some changes are not applied yet. Leave and put the previous values back?"),
		[WeakThis](bool bConfirmed)
		{
			UCoreKitBasicUISettingsMenu* Menu = WeakThis.Get();
			if (!bConfirmed || !Menu)
			{
				return;
			}
			Menu->Draft->Discard(Menu);
			Menu->bClosing = true;
			CoreKitBasicUIFrontEnd::CloseScreen(Menu);
		});
}

void UCoreKitBasicUISettingsMenu::SetStatus(const FText& Text, const FLinearColor& Color)
{
	if (PanelStatus)
	{
		PanelStatus->SetText(Text);
		PanelStatus->SetColorAndOpacity(FSlateColor(Color));
		PanelStatus->SetRenderOpacity(1.f);
	}
	StatusTime = 3.f;
}

// ---------------------------------------------------------------------------------------------------------------
// Rebinding (D7): UCoreKitNavigationSubsystem key capture + UCoreKitKeyBindings

namespace
{
	UEnhancedInputUserSettings* UserSettingsOf(const UUserWidget* Widget)
	{
		ULocalPlayer* Player = Widget ? Widget->GetOwningLocalPlayer() : nullptr;
		UEnhancedInputLocalPlayerSubsystem* Input = Player ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
		return Input ? Input->GetUserSettings() : nullptr;
	}

	FName MappingNameOf(const UInputMappingContext* Context, const UInputAction* Action)
	{
		if (Context && Action)
		{
			for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
			{
				if (Mapping.Action == Action && Mapping.IsPlayerMappable())
				{
					return Mapping.GetMappingName();
				}
			}
		}
		return NAME_None;
	}

	/** The player's mapping of the row for the device family of Key (lowest slot first). */
	bool FindPlayerMapping(UEnhancedInputUserSettings* Settings, FName MappingName, bool bGamepad, FMapPlayerKeyArgs& Out)
	{
		const UEnhancedPlayerMappableKeyProfile* Profile = Settings ? Settings->GetActiveKeyProfile() : nullptr;
		const FKeyMappingRow* Row = Profile ? Profile->FindKeyMappingRow(MappingName) : nullptr;
		if (!Row)
		{
			return false;
		}
		const FPlayerKeyMapping* Best = nullptr;
		for (const FPlayerKeyMapping& Mapping : Row->Mappings)
		{
			const FKey Reference = Mapping.GetCurrentKey().IsValid() ? Mapping.GetCurrentKey() : Mapping.GetDefaultKey();
			if (Reference.IsGamepadKey() == bGamepad && (!Best || Mapping.GetSlot() < Best->GetSlot()))
			{
				Best = &Mapping;
			}
		}
		if (!Best)
		{
			return false;
		}
		Out.MappingName = MappingName;
		Out.Slot = Best->GetSlot();
		Out.HardwareDeviceId = Best->GetHardwareDeviceId().HardwareDeviceIdentifier;
		return true;
	}
}

void UCoreKitBasicUISettingsMenu::BeginListening(int32 RowIndex)
{
	if (!Rows.IsValidIndex(RowIndex) || IsListening())
	{
		return;
	}
	const FRow& Row = Rows[RowIndex];
	if (!UserSettingsOf(this))
	{
		SetStatus(COREKIT_BASICUI_TEXT("NoUserSettings", "REBINDING IS OFF: ENHANCED INPUT USER SETTINGS ARE DISABLED"), CoreKitBasicUIColor::Red);
		return;
	}
	if (MappingNameOf(MappingContext, Row.InputAction).IsNone())
	{
		SetStatus(COREKIT_BASICUI_TEXT("NotMappable", "THIS ACTION IS NOT PLAYER-MAPPABLE YET"), CoreKitBasicUIColor::Red);
		return;
	}
	ULocalPlayer* Player = GetOwningLocalPlayer();
	UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	if (!Nav)
	{
		return;
	}
	Nav->OnKeyCaptureFinished.AddUniqueDynamic(this, &ThisClass::HandleKeyCaptured);
	if (!Nav->BeginKeyCapture())
	{
		Nav->OnKeyCaptureFinished.RemoveDynamic(this, &ThisClass::HandleKeyCaptured);
		return;
	}
	ListeningRow = RowIndex;
	PlayCue(TEXT("UI.Select"));
	RefreshVisuals();
}

void UCoreKitBasicUISettingsMenu::StopListening()
{
	ULocalPlayer* Player = GetOwningLocalPlayer();
	UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	if (Nav)
	{
		Nav->OnKeyCaptureFinished.RemoveDynamic(this, &ThisClass::HandleKeyCaptured);
		if (IsListening())
		{
			Nav->CancelKeyCapture();
		}
	}
	ListeningRow = INDEX_NONE;
	RefreshVisuals();
}

void UCoreKitBasicUISettingsMenu::HandleKeyCaptured(bool bCaptured, FKey Key)
{
	const int32 RowIndex = ListeningRow;
	ListeningRow = INDEX_NONE;
	if (ULocalPlayer* Player = GetOwningLocalPlayer())
	{
		Player->GetSubsystem<UCoreKitNavigationSubsystem>()->OnKeyCaptureFinished.RemoveDynamic(this, &ThisClass::HandleKeyCaptured);
	}
	RefreshVisuals();
	if (!Rows.IsValidIndex(RowIndex))
	{
		return;
	}
	if (!bCaptured)
	{
		SetStatus(COREKIT_BASICUI_TEXT("Cancelled", "CANCELLED"), CoreKitBasicUIColor::Muted);
		PlayCue(TEXT("UI.Back"));
		return;
	}
	if (Key == EKeys::Gamepad_Special_Right)
	{
		SetStatus(COREKIT_BASICUI_TEXT("Reserved", "THAT BUTTON IS RESERVED FOR PAUSE"), CoreKitBasicUIColor::Red);
		return;
	}
	const FRow& Row = Rows[RowIndex];
	UEnhancedInputUserSettings* Settings = UserSettingsOf(this);
	FMapPlayerKeyArgs Args;
	if (!FindPlayerMapping(Settings, MappingNameOf(MappingContext, Row.InputAction), Key.IsGamepadKey(), Args))
	{
		SetStatus(Key.IsGamepadKey() ? COREKIT_BASICUI_TEXT("NoPadSlot", "THIS ACTION HAS NO GAMEPAD BINDING") : COREKIT_BASICUI_TEXT("NoKeySlot", "THIS ACTION HAS NO KEYBOARD BINDING"), CoreKitBasicUIColor::Red);
		return;
	}
	Args.NewKey = Key;
	ULocalPlayer* Player = GetOwningLocalPlayer();
	const TArray<UInputMappingContext*> Concurrent;
	FCoreKitBindingReport Report = UCoreKitKeyBindings::ApplyKeyBinding(Player, MappingContext, Args, Concurrent, ECoreKitBindingDevice::Any, false);
	FText SwappedWith;
	if (Report.Result == ECoreKitBindingResult::Conflict && Report.ConflictingMappings.Num() == 1)
	{
		// A key already in use swaps with that action (D7).
		const FName Other = Report.ConflictingMappings[0];
		SwappedWith = FText::FromName(Other);
		for (const FRow& Candidate : Rows)
		{
			if (Candidate.InputAction && MappingNameOf(MappingContext, Candidate.InputAction) == Other)
			{
				SwappedWith = Candidate.Info.Label;
			}
		}
		Report = UCoreKitKeyBindings::ApplyKeyBinding(Player, MappingContext, Args, Concurrent, ECoreKitBindingDevice::Any, true);
	}
	const FText KeyName = Key.GetDisplayName(false).ToUpper();
	switch (Report.Result)
	{
	case ECoreKitBindingResult::Success:
		PlayCue(TEXT("UI.Confirm"));
		SetStatus(SwappedWith.IsEmpty() ? FText::Format(COREKIT_BASICUI_TEXT("Bound", "{0} BOUND TO {1}"), Row.Info.Label.ToUpper(), KeyName)
			: FText::Format(COREKIT_BASICUI_TEXT("Swapped", "{0} BOUND TO {1} · SWAPPED WITH {2}"), Row.Info.Label.ToUpper(), KeyName, SwappedWith.ToUpper()), CoreKitBasicUIColor::Mint);
		break;
	case ECoreKitBindingResult::Conflict:
		SetStatus(FText::Format(COREKIT_BASICUI_TEXT("InUse", "{0} IS USED BY {1} ACTIONS · NOT CHANGED"), KeyName, FText::AsNumber(Report.ConflictingMappings.Num())), CoreKitBasicUIColor::Red);
		break;
	default:
		SetStatus(COREKIT_BASICUI_TEXT("Rejected", "THAT KEY CANNOT BE USED HERE"), CoreKitBasicUIColor::Red);
		break;
	}
	if (Row.Keycap)
	{
		Row.Keycap->Refresh();
	}
}

void UCoreKitBasicUISettingsMenu::ResetRowBinding(int32 RowIndex)
{
	if (!Rows.IsValidIndex(RowIndex))
	{
		return;
	}
	const FRow& Row = Rows[RowIndex];
	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingNameOf(MappingContext, Row.InputAction);
	if (Args.MappingName.IsNone())
	{
		return;
	}
	if (UCoreKitKeyBindings::RestoreBindingDefaults(GetOwningLocalPlayer(), Args) == ECoreKitBindingResult::Success)
	{
		PlayCue(TEXT("UI.Confirm"));
		SetStatus(FText::Format(COREKIT_BASICUI_TEXT("RowReset", "{0} RESET TO DEFAULT"), Row.Info.Label.ToUpper()), CoreKitBasicUIColor::Mint);
		if (Row.Keycap)
		{
			Row.Keycap->Refresh();
		}
	}
}

void UCoreKitBasicUISettingsMenu::ResetAllBindings()
{
	for (const FRow& Row : Rows)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = MappingNameOf(MappingContext, Row.InputAction);
		if (!Args.MappingName.IsNone())
		{
			UCoreKitKeyBindings::RestoreBindingDefaults(GetOwningLocalPlayer(), Args);
			if (Row.Keycap)
			{
				Row.Keycap->Refresh();
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Motion

void UCoreKitBasicUISettingsMenu::TickMotion(float DeltaSeconds)
{
	if (bRestoreFocus)
	{
		bRestoreFocus = false;
		FocusRow(FocusedRow);
	}
	// Follow mouse hover / stick focus.
	const int32 Focused = FindFocusedRow();
	if (Focused != INDEX_NONE && Focused != FocusedRow)
	{
		FocusedRow = Focused;
		RefreshVisuals();
		RefreshSidePanel();
	}
	if (ContentIn.Tick(DeltaSeconds) || ContentIn.IsActive())
	{
		CoreKitBasicUIFrontEndUI::Enter(Content, ContentIn.Value, FVector2D(0.f, 12.f));
	}
	else if (Content)
	{
		CoreKitBasicUIFrontEndUI::Enter(Content, 1.f, FVector2D::ZeroVector);
	}
	if (StatusTime > 0.f)
	{
		StatusTime -= DeltaSeconds;
		if (StatusTime <= 0.f && PanelStatus)
		{
			PanelStatus->SetText(FText::GetEmpty());
		}
	}
}

#undef LOCTEXT_NAMESPACE
