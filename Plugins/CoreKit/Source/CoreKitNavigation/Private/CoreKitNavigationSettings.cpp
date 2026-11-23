#include "CoreKitNavigation/CoreKitNavigationSettings.h"

#include "CoreKitNavigation/CoreKitInputIconSet.h"
#include "GameFramework/InputSettings.h"
#include "InputCoreTypes.h"

UCoreKitNavigationSettings::UCoreKitNavigationSettings()
{
	HardwareSubstrings = { TEXT("DualSense"), TEXT("DualShock"), TEXT("PlayStation"), TEXT("PS5"), TEXT("PS4"), TEXT("Switch"), TEXT("Xbox"), TEXT("XInput") };
	HardwareGamepadTypes = { TEXT("PlayStation"), TEXT("PlayStation"), TEXT("PlayStation"), TEXT("PlayStation"), TEXT("PlayStation"), TEXT("Switch"), TEXT("Xbox"), TEXT("Xbox") };
	MenuLayers = { { TEXT("Game"), 0 }, { TEXT("Menu"), 10 }, { TEXT("Modal"), 20 } };
	ReturnKeys = { EKeys::Escape, EKeys::Gamepad_FaceButton_Right };
	NextSectionKeys = { EKeys::E, EKeys::Gamepad_RightShoulder };
	PreviousSectionKeys = { EKeys::Q, EKeys::Gamepad_LeftShoulder };
	AcceptKeys = { EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom };
}

int32 UCoreKitNavigationSettings::GetLayerPriority(FName Layer) const
{
	const int32* Found = MenuLayers.Find(Layer);
	return Found ? *Found : 10;
}

FName UCoreKitNavigationSettings::ResolveGamepadType(const FHardwareDeviceIdentifier& Hardware) const
{
	const FString Device = Hardware.HardwareDeviceIdentifier.ToString();
	const FString Class = Hardware.InputClassName.ToString();
	const int32 Count = FMath::Min(HardwareSubstrings.Num(), HardwareGamepadTypes.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FString& Needle = HardwareSubstrings[Index];
		if (!Needle.IsEmpty() && (Device.Contains(Needle) || Class.Contains(Needle)))
		{
			return HardwareGamepadTypes[Index];
		}
	}
	return DefaultGamepadType;
}

UCoreKitInputIconSet* UCoreKitNavigationSettings::LoadIconSet(bool bGamepad, FName GamepadType) const
{
	if (!bGamepad)
	{
		return KeyboardMouseIcons.LoadSynchronous();
	}
	if (const TSoftObjectPtr<UCoreKitInputIconSet>* Set = GamepadIcons.Find(GamepadType); Set && !Set->IsNull())
	{
		return Set->LoadSynchronous();
	}
	if (const TSoftObjectPtr<UCoreKitInputIconSet>* Fallback = GamepadIcons.Find(DefaultGamepadType); Fallback && !Fallback->IsNull())
	{
		return Fallback->LoadSynchronous();
	}
	return nullptr;
}
