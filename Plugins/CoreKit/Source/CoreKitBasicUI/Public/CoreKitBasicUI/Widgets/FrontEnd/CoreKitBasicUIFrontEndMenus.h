#pragma once

#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICommonWidgets.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIFrontEndMenus.generated.h"

class UBorder;
class UCanvasPanel;
class UCoreKitButton;
class UCoreKitOptionSelector;
class UCoreKitSectionList;
class UCoreKitBasicUICardWidget;
class UCoreKitBasicUIUserSettings;
class UHorizontalBox;
class UImage;
class UInputAction;
class UInputMappingContext;
class UScrollBox;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

/*
 * Front-end screens (stage UI-7, Docs/UI/Spec/05-FrontEnd.md). Layouts are built natively (BuildTree) with the design
 * system; motion is tweened in TickMotion. The Pause menu opens How to Play and Settings on top of itself with
 * CoreKitBasicUIFrontEnd::Open*(Controller); Back returns to whoever opened them.
 */

/** Keycap with one fixed key per device family (footer hints: ESC / B, T / Y, F / X...). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIDeviceKeycap : public UCoreKitBasicUIKeycap
{
	GENERATED_BODY()

public:
	/** Shows KeyboardKey with keyboard/mouse and GamepadKey with a gamepad (an invalid gamepad key keeps the keyboard one). */
	void SetKeys(FKey InKeyboardKey, FKey InGamepadKey);
	/** Collapses the keycap (and its size box) while a gamepad is active (WASD: only the stick glyph remains). */
	void SetHiddenOnGamepad(bool bHide) { bHiddenOnGamepad = bHide; }

protected:
	void NativeConstruct() override;
	void NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey) override;

private:
	FKey WantedKey() const;

	FKey KeyboardKey;
	FKey GamepadKey;
	bool bSwapping = false;
	bool bHiddenOnGamepad = false;
};

/** One Settings row as declared by the screen (spec 05 table). Exposed for tests. */
UENUM()
enum class ECoreKitBasicUISettingsControl : uint8
{
	Slider,
	Segmented,
	Arrows,
	Swatches,
	Binding,
	Locked
};

USTRUCT()
struct COREKITBASICUI_API FCoreKitBasicUISettingsRowInfo
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() FText Label;
	UPROPERTY() FText Description;
	UPROPERTY() ECoreKitBasicUISettingsControl Control = ECoreKitBasicUISettingsControl::Segmented;
	/** Input action asset name for binding rows (IA_Jump...). */
	UPROPERTY() FName Action;
};

/**
 * Draft of the Settings screen on top of UCoreKitBasicUIUserSettings: edits are live on the target object, a committed snapshot
 * tells whether there are unapplied changes, Discard restores it and Commit applies + saves. Video values (display
 * mode, resolution, frame limit, V-Sync) are not previewed live (re-applying the window per step would flicker); they
 * apply on Commit.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUISettingsDraft : public UObject
{
	GENERATED_BODY()

public:
	void Begin(UCoreKitBasicUIUserSettings* InTarget);
	bool IsDirty() const;
	bool IsVideoDirty() const;
	/** Applies everything except the video values (UCoreKitBasicUIUserSettings::ApplyNonVideo). */
	void Preview(const UObject* WorldContext);
	void Commit(const UObject* WorldContext);
	void Discard(const UObject* WorldContext);
	/** Engine application can be disabled for tests. */
	bool bApplyToEngine = true;
	bool bSaveOnCommit = true;

	UCoreKitBasicUIUserSettings* GetTarget() const { return Target; }
	UCoreKitBasicUIUserSettings* GetCommitted() const { return Committed; }

	static void CopySettings(UCoreKitBasicUIUserSettings* To, const UCoreKitBasicUIUserSettings* From);
	static bool SameVideo(const UCoreKitBasicUIUserSettings* A, const UCoreKitBasicUIUserSettings* B);

private:
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIUserSettings> Target;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIUserSettings> Committed;
};

/**
 * SYSTEM CONFIG / SETTINGS: tabs GAMEPLAY | CONTROLS | VIDEO | AUDIO (Q/E, LB/RB), rows with sliders, segmented and
 * arrow selectors, crosshair swatches and key rebinding; side panel SELECTED with descriptions and a live crosshair
 * preview; footer BACK / T RESET (confirmation) / F APPLY. Changes preview live; Back with unapplied changes asks.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUISettingsMenu : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUISettingsMenu(const FObjectInitializer& ObjectInitializer);

	static const TArray<FCoreKitBasicUISettingsRowInfo>& GetRowInfos(int32 Tab);
	static TArray<FText> GetTabLabels();

	bool HasUnappliedChanges() const;
	int32 GetActiveTab() const;
	int32 GetFocusedRow() const { return FocusedRow; }
	bool IsListening() const { return ListeningRow != INDEX_NONE; }

	/** Commands (also bound to F / T / Back). */
	void ApplyChanges();
	void RequestReset();
	void RequestBack();

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeOnMenuActivated() override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;
	bool HandleMenuKey_Implementation(FKey Key) override;
	FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	struct FRow
	{
		FCoreKitBasicUISettingsRowInfo Info;
		int32 Tab = 0;
		// Widgets (owned by the WidgetTree).
		USizeBox* Root = nullptr;
		UImage* Background = nullptr;
		UImage* Inset = nullptr;
		UCoreKitButton* Button = nullptr;
		UTextBlock* Label = nullptr;
		UTextBlock* Value = nullptr;
		USlider* Slider = nullptr;
		USizeBox* SliderFill = nullptr;
		UImage* SliderFillImage = nullptr;
		UCoreKitOptionSelector* Selector = nullptr;
		TArray<UCoreKitButton*> Swatches;
		TArray<UImage*> SwatchRings;
		UCoreKitBasicUIKeycap* Keycap = nullptr;
		UTextBlock* Listening = nullptr;
		// Slider range.
		float Min = 0.f;
		float Max = 100.f;
		float Step = 1.f;
		float LastSlider = -1.f;
		int32 LastIndex = -1;
		// Binding.
		UInputAction* InputAction = nullptr;
	};

	void BuildRows(int32 Tab, UVerticalBox* Column);
	UWidget* BuildControl(FRow& Row);
	void StyleSelector(UCoreKitOptionSelector* Selector, UCoreKitButton* Button, int32 Index, bool bSelected);
	void StyleTab(UCoreKitButton* Button, int32 Index, bool bActive);

	// Values.
	float GetSliderValue(const FRow& Row) const;
	void SetSliderValue(FRow& Row, float Value);
	int32 GetOptionIndex(const FRow& Row) const;
	void SetOptionIndex(FRow& Row, int32 Index);
	TArray<FText> GetOptions(const FRow& Row) const;
	FText FormatValue(const FRow& Row) const;
	void RefreshRow(FRow& Row);
	void RefreshAllRows();
	void RefreshVisuals();
	void RefreshSidePanel();
	void OnValueEdited();

	// Navigation.
	TArray<int32> RowsOfTab(int32 Tab) const;
	void FocusRow(int32 RowIndex);
	void MoveFocus(int32 Delta);
	void StepRow(int32 RowIndex, int32 Delta);
	void ActivateRow(int32 RowIndex);
	int32 FindFocusedRow() const;

	// Rebinding.
	void BeginListening(int32 RowIndex);
	void StopListening();
	void ResetRowBinding(int32 RowIndex);
	void ResetAllBindings();
	UInputMappingContext* GetMappingContext() const;
	void SetStatus(const FText& Text, const FLinearColor& Color);

	UFUNCTION() void HandleSliderChanged(float Value);
	UFUNCTION() void HandleOptionChanged(int32 Index);
	UFUNCTION() void HandleRowActivated(UCoreKitButton* Button);
	UFUNCTION() void HandleSwatchActivated(UCoreKitButton* Button);
	UFUNCTION() void HandleSectionChanged(int32 Section);
	UFUNCTION() void HandleKeyCaptured(bool bCaptured, FKey Key);
	UFUNCTION() UWidget* HandleHorizontalNavigation(EUINavigation Direction);
	UFUNCTION() void HandleFooterBack(UCoreKitButton* Button);
	UFUNCTION() void HandleFooterReset(UCoreKitButton* Button);
	UFUNCTION() void HandleFooterApply(UCoreKitButton* Button);

	TArray<FRow> Rows;
	TArray<FIntPoint> Resolutions;

	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUISettingsDraft> Draft;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> Actions;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UCoreKitSectionList> Tabs;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TArray<TObjectPtr<UScrollBox>> Scrolls;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PanelKicker;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PanelTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PanelBody;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PanelHint;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PanelStatus;
	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> PreviewBlock;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> PreviewMarks;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ApplyLabel;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIDeviceKeycap> ApplyKeycap;
	UPROPERTY(Transient) TObjectPtr<UWidget> Content;

	FCoreKitBasicUITween ContentIn;
	float StatusTime = 0.f;
	int32 FocusedRow = INDEX_NONE;
	bool bRestoreFocus = false;
	int32 ListeningRow = INDEX_NONE;
	bool bClosing = false;
};

/** FIELD MANUAL / HOW TO PLAY: controls (current bindings), the loop, weapons, glow colours and card suits. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIHowToPlayMenu : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIHowToPlayMenu(const FObjectInitializer& ObjectInitializer);

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;
	void NativeConstruct() override;
	void NativeOnMenuActivated() override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;

private:
	UWidget* BuildControls();
	UWidget* BuildLoop();
	UWidget* BuildGlow();
	UWidget* ControlRow(const FText& Label, UWidget* Keys, bool bLast = false, const FLinearColor& LabelColor = CoreKitBasicUIColor::White);
	UCoreKitBasicUIKeycap* ActionKeycap(FName ActionName, FKey Fallback, const FLinearColor& Accent = CoreKitBasicUIColor::Steel);
	UInputAction* LoadAction(FName ActionName);

	UFUNCTION() void HandleBack(UCoreKitButton* Button);

	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> Actions;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Columns;
	UPROPERTY(Transient) TObjectPtr<UCoreKitButton> BackButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DashNote;

	float IntroTime = -1.f;
};

/** Credits (D2): jam team, third-party attributions (Kenney CC0 packs, OFL fonts) and tools, in the design system. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUICreditsMenu : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUICreditsMenu(const FObjectInitializer& ObjectInitializer);

	/** Team name shown in the credits (no team name is recorded in the GCoreKitBasicUI or project docs). */
	static FText GetTeamName();
	/** Kenney packs credited (names as in ArtSource/ThirdParty/Manifest.json). */
	static TArray<FString> GetKenneyPacks();
	/** Font families credited under the SIL Open Font License 1.1. */
	static TArray<FString> GetFontFamilies();

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;
	void NativeOnMenuActivated() override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;

private:
	UFUNCTION() void HandleBack(UCoreKitButton* Button);

	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Columns;
	UPROPERTY(Transient) TObjectPtr<UCoreKitButton> BackButton;
	float IntroTime = -1.f;
};

/**
 * Title screen on L_MainMenu: logo plate, taglines, decorative card fan, PLAY / HOW TO PLAY / SETTINGS / CREDITS / QUIT,
 * mission brief, footer with navigation glyphs and the build version, intro sequence (~1.5 s). PLAY starts the
 * CoreKitFrontend New Game after a cover wipe; QUIT asks for confirmation.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIMainMenu : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIMainMenu(const FObjectInitializer& ObjectInitializer);

	/** Restarts the intro sequence. */
	void PlayIntro();
	bool IsIntroPlaying() const;
	/** Entry labels in order (PLAY, HOW TO PLAY, SETTINGS, CREDITS, QUIT). */
	static TArray<FText> GetEntryLabels();

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeOnMenuActivated() override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;
	bool HandleMenuKey_Implementation(FKey Key) override;

private:
	UWidget* BuildLogo();
	UWidget* BuildFan();
	UWidget* BuildBrief();
	UWidget* BuildFooter();
	void ApplyIntro(float T);
	void HandleEntry(UCoreKitBasicUIActionButton* Button);
	void StartNewRun();
	void MoveSelection(int32 Delta);

	UFUNCTION() void HandleFrontendChanged();

	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIActionButton>> Entries;
	UPROPERTY(Transient) TObjectPtr<UImage> FadeIn;
	UPROPERTY(Transient) TObjectPtr<UWidget> Kicker;
	UPROPERTY(Transient) TObjectPtr<UWidget> DeadPlate;
	UPROPERTY(Transient) TObjectPtr<UWidget> DrawPlate;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Bolts;
	UPROPERTY(Transient) TObjectPtr<UWidget> TapeRow;
	UPROPERTY(Transient) TObjectPtr<UWidget> Tagline;
	UPROPERTY(Transient) TObjectPtr<UWidget> Fan;
	UPROPERTY(Transient) TObjectPtr<UWidget> Brief;
	UPROPERTY(Transient) TObjectPtr<UWidget> Footer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;

	float IntroTime = -1.f;
	int32 LastEntry = 0;
	bool bIntroPlayed = false;
	bool bStarting = false;
	bool bRestoreFocus = false;
};

/** Screen wipe of Docs/UI/Spec/07-Motion.md: a magenta slash (8 px, 12 degrees, glow) crosses left to right in 320 ms DEAL. */
UENUM()
enum class ECoreKitBasicUIWipeMode : uint8
{
	/** Soot covers the screen and the slash uncovers the new screen behind it (screen changes, arrival). */
	Reveal,
	/** The slash crosses and leaves Soot behind it (before map travel; the loading screen continues it). */
	Cover
};

UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIScreenWipe : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	/** Adds a wipe on top of the player's screen. OnDone runs once at the end (a Cover wipe then stays on screen). */
	static UCoreKitBasicUIScreenWipe* Play(APlayerController* Controller, ECoreKitBasicUIWipeMode Mode, TFunction<void()> OnDone = {});
	static float GetDuration();

	ECoreKitBasicUIWipeMode GetMode() const { return Mode; }
	bool IsFinished() const { return bFinished; }

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;

private:
	void Layout(float Alpha);

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Root;
	UPROPERTY(Transient) TObjectPtr<UImage> Cover;
	UPROPERTY(Transient) TObjectPtr<UImage> Glow;
	UPROPERTY(Transient) TObjectPtr<UImage> Line;
	UPROPERTY(Transient) TObjectPtr<UImage> Core;

	TFunction<void()> Done;
	ECoreKitBasicUIWipeMode Mode = ECoreKitBasicUIWipeMode::Reveal;
	float Elapsed = 0.f;
	bool bFinished = false;
};

/** CoreKitLoading screen between maps: the frame a Cover wipe ends on (Soot) with a small logo and a pulsing status. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUILoadingScreen : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float DeltaSeconds) override;

private:
	UPROPERTY(Transient) TObjectPtr<UWidget> Pulse;
	float Time = 0.f;
};

namespace CoreKitBasicUIFrontEnd
{
	COREKITBASICUI_API UCoreKitBasicUISettingsMenu* OpenSettings(APlayerController* Controller);
	COREKITBASICUI_API UCoreKitBasicUIHowToPlayMenu* OpenHowToPlay(APlayerController* Controller);
	COREKITBASICUI_API UCoreKitBasicUICreditsMenu* OpenCredits(APlayerController* Controller);

	/** Closes a front-end screen with a reveal wipe over whatever is below it. */
	COREKITBASICUI_API void CloseScreen(UCoreKitMenuWidget* Screen);
	/** "JAM BUILD 0.1 // UNREAL ENGINE 5" from [/Script/EngineSettings.GeneralProjectSettings] ProjectVersion. */
	COREKITBASICUI_API FText GetBuildVersionText();
	/** Menu music (D4): the combat track of DA_RunMusic, low-passed, at this volume. */
	COREKITBASICUI_API float GetMenuMusicVolume();
}
