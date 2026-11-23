#pragma once

#include "InputCoreTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CoreKitNavigation/CoreKitInputIconSet.h"
#include "CoreKitNavigation/CoreKitNavigationTypes.h"
#include "CoreKitNavigation/CoreKitPromptWidget.h"
#include "CoreKitNavigationSubsystem.generated.h"

class FCoreKitNavigationInput;
class SWidget;
class UCoreKitMenuWidget;
class UCoreKitNavigationGlyphs;
class UCoreKitPromptWidget;
class UInputAction;
class UTexture2D;
class UUserWidget;
class UWidget;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitInputDeviceChanged, ECoreKitInputDevice, Device);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitGamepadTypeChanged, FName, GamepadType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitKeyCaptureFinished, bool, bCaptured, FKey, Key);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitTopMenuChanged, UCoreKitMenuWidget*, Menu);

UCLASS()
class COREKITNAVIGATION_API UCoreKitNavigationSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetDeviceTrackingEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	ECoreKitInputDevice GetInputDevice() const { return InputDevice; }

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitInputDeviceChanged OnInputDeviceChanged;

	/** Focus a widget already attached to Slate and remember the previous focus. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	bool PushMenuFocus(UWidget* InitialWidget);

	/** Restore the previous focus, falling back to the game viewport. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void PopMenuFocus();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	FGuid PushMenuFocusScope(UWidget* InitialWidget, UUserWidget* Menu = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	bool ReleaseMenuFocusScope(FGuid Id);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	bool BeginKeyCapture();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	bool CancelKeyCapture();

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitKeyCaptureFinished OnKeyCaptureFinished;

	/** Returns the first mapped glyph for the current device. Keyboard and mouse share bindings. */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	UTexture2D* FindActionGlyph(const UInputAction* Action, const UCoreKitNavigationGlyphs* Glyphs) const;

	/** Gamepad type used for glyphs: the player's override if set, else the last identified hardware, else the project default. */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	FName GetGamepadType() const;

	/** Player preference for gamepad glyphs. Pass None to return to automatic detection. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetGamepadTypeOverride(FName GamepadType);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	FName GetGamepadTypeOverride() const { return GamepadTypeOverride; }

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation")
	FCoreKitGamepadTypeChanged OnGamepadTypeChanged;

	/**
	 * Presentation for one key from the project's icon sets (CoreKit Navigation settings).
	 * Gamepad keys use the current gamepad type. Falls back to the key's display name when no set has it.
	 */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	FCoreKitKeyPresentation FindKeyPresentation(FKey Key) const;

	/**
	 * Presentation of the first key mapped to the action for the current device family.
	 * Returns false when the action has no key for that family; OutKey is then invalid.
	 */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	bool FindActionPresentation(const UInputAction* Action, FKey& OutKey, FCoreKitKeyPresentation& OutPresentation) const;

	/** Re-reads the most recently used hardware device and updates the gamepad type. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void RefreshGamepadType();

	/**
	 * Adds a menu to the player's screen on a layer (see CoreKit Navigation settings) and makes the top menu
	 * own focus, input mode, cursor, Return and section input. Enables device tracking.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation|Menus")
	bool PushMenu(UCoreKitMenuWidget* Menu, FName Layer = "Menu");

	/** Removes a menu (after its Anim_Out) and restores the previous top menu or the game's input. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation|Menus")
	bool PopMenu(UCoreKitMenuWidget* Menu);

	/** Pops every menu on a layer, or all menus when Layer is None. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation|Menus")
	void PopAllMenus(FName Layer = NAME_None);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation|Menus")
	UCoreKitMenuWidget* GetTopMenu() const;

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation|Menus")
	UCoreKitMenuWidget* GetTopMenuInLayer(FName Layer) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation|Menus")
	int32 GetMenuCount() const { return Menus.Num(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation|Menus")
	bool IsMenuOpen(const UCoreKitMenuWidget* Menu) const;

	/** Creates and pushes a prompt on the Modal layer. The callback runs once with the chosen option. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation|Menus", meta = (AutoCreateRefTerm = "Options"))
	UCoreKitPromptWidget* PushPrompt(TSubclassOf<UCoreKitPromptWidget> PromptClass, FText Title, FText Body,
		const TArray<FText>& Options, int32 CancelOption, int32 DefaultOption, FCoreKitPromptCallback Callback);

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Navigation|Menus")
	FCoreKitTopMenuChanged OnTopMenuChanged;

private:
	friend class FCoreKitNavigationInput;

	void SetDetectedGamepadType(FName GamepadType);
	void ObserveInput(uint32 SlateUserIndex, ECoreKitInputDevice Device);
	bool CaptureKey(uint32 SlateUserIndex, FKey Key);
	void HandleMenuDestruct(UUserWidget* Menu, FGuid Id);
	bool HandleMenuKey(uint32 SlateUserIndex, FKey Key, bool bRepeat);
	void RefreshTopMenu();
	void ApplyMenuInput();
	APlayerController* GetMenuPlayerController() const;

	struct FMenuEntry
	{
		TWeakObjectPtr<UCoreKitMenuWidget> Menu;
		FName Layer;
		int32 Priority = 0;
		int32 Order = 0;
		FGuid FocusScope;
	};
	TArray<FMenuEntry> Menus;
	TWeakObjectPtr<UCoreKitMenuWidget> ActiveMenu;
	int32 NextMenuOrder = 0;
	bool bMenuInputApplied = false;
	/** Map travel: menus of the old world are gone, so the new controller gets game input back. */
	void HandlePostLoadMap(UWorld* LoadedWorld);
	FDelegateHandle PostLoadMapHandle;
	bool bSavedShowCursor = false;

	struct FMenuFocusScope
	{
		FGuid Id;
		TWeakObjectPtr<UUserWidget> Menu;
		TWeakPtr<SWidget> Target;
		TWeakPtr<SWidget> Previous;
		FDelegateHandle OnDestruct;
	};

	TSharedPtr<FCoreKitNavigationInput> InputProcessor;
	TArray<FMenuFocusScope> FocusScopes;
	ECoreKitInputDevice InputDevice = ECoreKitInputDevice::Keyboard;
	FName DetectedGamepadType;
	FName GamepadTypeOverride;
	bool bCapturingKey = false;
};
