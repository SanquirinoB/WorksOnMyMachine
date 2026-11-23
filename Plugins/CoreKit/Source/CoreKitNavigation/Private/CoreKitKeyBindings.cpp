#include "CoreKitNavigation/CoreKitKeyBindings.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

namespace
{
UEnhancedInputUserSettings* GetSettings(ULocalPlayer* Player)
{
	auto* Input = IsValid(Player) ? Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return Input ? Input->GetUserSettings() : nullptr;
}

UEnhancedPlayerMappableKeyProfile* GetProfile(UEnhancedInputUserSettings& Settings, const FMapPlayerKeyArgs& Binding)
{
	return Binding.ProfileIdString.IsEmpty() ? Settings.GetActiveKeyProfile() : Settings.GetKeyProfileWithId(Binding.ProfileIdString);
}

void AddMappingNames(const UInputMappingContext* Context, TSet<FName>& Names)
{
	if (IsValid(Context))
	{
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.IsPlayerMappable())
			{
				Names.Add(Mapping.GetMappingName());
			}
		}
	}
}

bool MatchesDevice(FKey Key, ECoreKitBindingDevice Device)
{
	return Device == ECoreKitBindingDevice::Any || Key.IsGamepadKey() == (Device == ECoreKitBindingDevice::Gamepad);
}

TArray<const FPlayerKeyMapping*> FindConflicts(const UEnhancedPlayerMappableKeyProfile& Profile,
	const TSet<FName>& Names, FKey Key, const FPlayerKeyMapping* Except, const FPlayerKeyMapping* AlsoExcept = nullptr)
{
	TArray<const FPlayerKeyMapping*> Conflicts;
	for (const FName Name : Names)
	{
		if (const FKeyMappingRow* Row = Profile.FindKeyMappingRow(Name))
		{
			for (const FPlayerKeyMapping& Mapping : Row->Mappings)
			{
				if (&Mapping != Except && &Mapping != AlsoExcept && Mapping.GetCurrentKey() == Key)
				{
					Conflicts.Add(&Mapping);
				}
			}
		}
	}
	return Conflicts;
}

bool MapKey(UEnhancedInputUserSettings& Settings, const FMapPlayerKeyArgs& Binding)
{
	FGameplayTagContainer Errors;
	Settings.MapPlayerKey(Binding, Errors);
	return Errors.IsEmpty();
}
}

FCoreKitBindingReport UCoreKitKeyBindings::ApplyKeyBinding(ULocalPlayer* Player, UInputMappingContext* Context,
	const FMapPlayerKeyArgs& Binding, const TArray<UInputMappingContext*>& ConcurrentContexts,
	ECoreKitBindingDevice Device, bool bSwap)
{
	UEnhancedInputUserSettings* Settings = GetSettings(Player);
	UEnhancedPlayerMappableKeyProfile* Profile = Settings ? GetProfile(*Settings, Binding) : nullptr;
	const FPlayerKeyMapping* Target = Profile ? Profile->FindKeyMapping(Binding) : nullptr;
	TSet<FName> Names;
	AddMappingNames(Context, Names);
	if (!Target || !Names.Contains(Binding.MappingName) || !Binding.NewKey.IsValid())
	{
		return {};
	}
	if (!MatchesDevice(Binding.NewKey, Device))
	{
		return { ECoreKitBindingResult::WrongDevice };
	}
	if (Target->GetCurrentKey() == Binding.NewKey)
	{
		return { ECoreKitBindingResult::Success };
	}

	for (const auto* Concurrent : ConcurrentContexts)
	{
		AddMappingNames(Concurrent, Names);
	}

	const auto Conflicts = FindConflicts(*Profile, Names, Binding.NewKey, Target);
	FCoreKitBindingReport Report;
	Report.Result = ECoreKitBindingResult::Conflict;
	for (const auto* Conflict : Conflicts)
	{
		Report.ConflictingMappings.AddUnique(Conflict->GetMappingName());
	}
	Report.ConflictingMappings.Sort(FNameLexicalLess());

	FMapPlayerKeyArgs Primary = Binding;
	Primary.bCreateMatchingSlotIfNeeded = false;
	Primary.bDeferOnSettingsChangedBroadcast = true;
	const FKey PreviousKey = Target->GetCurrentKey();

	FMapPlayerKeyArgs Secondary;
	if (!Conflicts.IsEmpty())
	{
		if (!bSwap || Conflicts.Num() != 1 || !PreviousKey.IsValid()
			|| PreviousKey.IsGamepadKey() != Binding.NewKey.IsGamepadKey()
			|| !FindConflicts(*Profile, Names, PreviousKey, Target, Conflicts[0]).IsEmpty())
		{
			return Report;
		}

		const FPlayerKeyMapping& Other = *Conflicts[0];
		Secondary.MappingName = Other.GetMappingName();
		Secondary.Slot = Other.GetSlot();
		Secondary.HardwareDeviceId = Other.GetHardwareDeviceId().HardwareDeviceIdentifier;
		Secondary.ProfileIdString = Binding.ProfileIdString;
		Secondary.NewKey = PreviousKey;
		Secondary.bCreateMatchingSlotIfNeeded = false;
		Secondary.bDeferOnSettingsChangedBroadcast = true;
	}

	if (!MapKey(*Settings, Primary))
	{
		return { ECoreKitBindingResult::SettingsRejected };
	}
	if (!Conflicts.IsEmpty() && !MapKey(*Settings, Secondary))
	{
		Primary.NewKey = PreviousKey;
		MapKey(*Settings, Primary);
		return { ECoreKitBindingResult::SettingsRejected };
	}

	Settings->SaveSettings();
	return { ECoreKitBindingResult::Success };
}

ECoreKitBindingResult UCoreKitKeyBindings::RestoreBindingDefaults(ULocalPlayer* Player, const FMapPlayerKeyArgs& Binding)
{
	UEnhancedInputUserSettings* Settings = GetSettings(Player);
	if (!Settings)
	{
		return ECoreKitBindingResult::InvalidBinding;
	}

	FGameplayTagContainer Errors;
	Settings->ResetAllPlayerKeysInRow(Binding, Errors);
	if (!Errors.IsEmpty())
	{
		return ECoreKitBindingResult::SettingsRejected;
	}
	Settings->SaveSettings();
	return ECoreKitBindingResult::Success;
}
