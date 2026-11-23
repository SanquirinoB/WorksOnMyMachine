#include "CoreKitData/CoreKitDataContainer.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UCoreKitDataFragment* UCoreKitDataContainer::FindDataFragment(TSubclassOf<UCoreKitDataFragment> FragmentClass) const
{
	const UClass* RequestedClass = FragmentClass.Get();
	if (!RequestedClass)
	{
		return nullptr;
	}

	for (UCoreKitDataFragment* Fragment : Fragments)
	{
		if (IsValid(Fragment) && Fragment->IsA(RequestedClass))
		{
			return Fragment;
		}
	}

	return nullptr;
}

TArray<UCoreKitDataFragment*> UCoreKitDataContainer::FindDataFragments(TSubclassOf<UCoreKitDataFragment> FragmentClass) const
{
	TArray<UCoreKitDataFragment*> Matches;
	const UClass* RequestedClass = FragmentClass.Get();
	if (!RequestedClass)
	{
		return Matches;
	}

	for (UCoreKitDataFragment* Fragment : Fragments)
	{
		if (IsValid(Fragment) && Fragment->IsA(RequestedClass))
		{
			Matches.Add(Fragment);
		}
	}

	return Matches;
}

#if WITH_EDITOR
EDataValidationResult UCoreKitDataContainer::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	for (int32 Index = 0; Index < Fragments.Num(); ++Index)
	{
		if (!IsValid(Fragments[Index]))
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("CoreKitData", "MissingFragment", "Select a data fragment at index {0}, or remove the empty entry."),
				FText::AsNumber(Index)));
			Result = EDataValidationResult::Invalid;
		}
	}

	TSet<const UClass*> RuleClasses;
	for (const FCoreKitFragmentRule& Rule : FragmentRules)
	{
		const UClass* Class = Rule.FragmentClass.Get();
		if (!Class || RuleClasses.Contains(Class) || Rule.Minimum < 0 || Rule.Maximum < -1
			|| (Rule.Maximum >= 0 && Rule.Maximum < Rule.Minimum))
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("CoreKitData", "InvalidFragmentRule", "Invalid or duplicate fragment rule for {0}."),
				FText::FromString(GetNameSafe(Class))));
			Result = EDataValidationResult::Invalid;
			continue;
		}
		RuleClasses.Add(Class);
		const int32 Count = FindDataFragments(Rule.FragmentClass).Num();
		if (Count < Rule.Minimum)
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("CoreKitData", "FragmentMinimum", "{0} requires at least {1} matching fragments. Found {2}."),
				Class->GetDisplayNameText(), Rule.Minimum, Count));
			Result = EDataValidationResult::Invalid;
		}
		if (Rule.Maximum >= 0 && Count > Rule.Maximum)
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("CoreKitData", "FragmentMaximum", "{0} allows at most {1} matching fragments. Found {2}."),
				Class->GetDisplayNameText(), Rule.Maximum, Count));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif
