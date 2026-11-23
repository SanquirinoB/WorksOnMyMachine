#pragma once

#include "Engine/DeveloperSettings.h"
#include "CoreKitNavigation/CoreKitNavigationTypes.h"
#include "CoreKitNavigationSettings.generated.h"

class UCoreKitInputIconSet;
class USoundBase;
struct FHardwareDeviceIdentifier;

/** Project-wide navigation presentation. Mutable runtime state lives in UCoreKitNavigationSubsystem. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CoreKit Navigation"))
class COREKITNAVIGATION_API UCoreKitNavigationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCoreKitNavigationSettings();

	FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Icons for keyboard keys and mouse buttons. */
	UPROPERTY(Config, EditAnywhere, Category = "Glyphs")
	TSoftObjectPtr<UCoreKitInputIconSet> KeyboardMouseIcons;

	/** Icons for gamepad keys, by gamepad type (for example Xbox, PlayStation, Switch, Generic). */
	UPROPERTY(Config, EditAnywhere, Category = "Glyphs")
	TMap<FName, TSoftObjectPtr<UCoreKitInputIconSet>> GamepadIcons;

	/** Used when the hardware cannot be identified, for example plain XInput on Windows. */
	UPROPERTY(Config, EditAnywhere, Category = "Glyphs")
	FName DefaultGamepadType = TEXT("Xbox");

	/** Case-insensitive substrings of the hardware identifier or input class, mapped to a gamepad type. First match wins. */
	UPROPERTY(Config, EditAnywhere, Category = "Glyphs")
	TArray<FString> HardwareSubstrings;

	/** Parallel to HardwareSubstrings. */
	UPROPERTY(Config, EditAnywhere, Category = "Glyphs")
	TArray<FName> HardwareGamepadTypes;

	/** Menu layers by name and priority. The highest-priority layer with menus owns menu input. */
	UPROPERTY(Config, EditAnywhere, Category = "Menus")
	TMap<FName, int32> MenuLayers;

	/** Keys that trigger Return/Back on the top menu. Handled before widgets, also while the game is paused. */
	UPROPERTY(Config, EditAnywhere, Category = "Menus")
	TArray<FKey> ReturnKeys;

	UPROPERTY(Config, EditAnywhere, Category = "Menus")
	TArray<FKey> NextSectionKeys;

	UPROPERTY(Config, EditAnywhere, Category = "Menus")
	TArray<FKey> PreviousSectionKeys;

	/** Keys swallowed while the top menu is in its activation cooldown. */
	UPROPERTY(Config, EditAnywhere, Category = "Menus")
	TArray<FKey> AcceptKeys;

	/** Seconds after a menu becomes the top menu during which Select and Return are ignored. */
	UPROPERTY(Config, EditAnywhere, Category = "Menus", meta = (ClampMin = "0"))
	float DefaultActivationCooldown = 0.15f;

	/** Hide the cursor while the player uses a gamepad in a menu that shows it. */
	UPROPERTY(Config, EditAnywhere, Category = "Menus")
	bool bAutoHideCursorWithGamepad = true;

	UPROPERTY(Config, EditAnywhere, Category = "Sounds")
	TSoftObjectPtr<USoundBase> NavigateSound;

	UPROPERTY(Config, EditAnywhere, Category = "Sounds")
	TSoftObjectPtr<USoundBase> ActivateSound;

	UPROPERTY(Config, EditAnywhere, Category = "Sounds")
	TSoftObjectPtr<USoundBase> ReturnSound;

	int32 GetLayerPriority(FName Layer) const;

	/** Resolves a gamepad type from a hardware identifier, falling back to DefaultGamepadType. */
	FName ResolveGamepadType(const FHardwareDeviceIdentifier& Hardware) const;

	/** Loads the icon set for a device family. Keyboard and Mouse share one set. */
	UCoreKitInputIconSet* LoadIconSet(bool bGamepad, FName GamepadType) const;
};
