#include "CoreKitActions/States/CoreKitStateProfile.h"

bool UCoreKitStateProfile::ValidateProfile(FString& Reason) const
{
	TSet<FName> Names;
	FGameplayTagContainer Reserved;
	for (const auto& Domain : Domains)
	{
		if (!Domain.Validate(Reason)) { return false; }
		if (Names.Contains(Domain.Domain))
		{
			Reason = TEXT("Duplicate domain name.");
			return false;
		}
		Names.Add(Domain.Domain);
		for (const auto& Tag : Domain.States)
		{
			for (const auto& Other : Reserved)
			{
				if (Tag.MatchesTag(Other) || Other.MatchesTag(Tag))
				{
					Reason = TEXT("Two domains cannot own overlapping state hierarchies.");
					return false;
				}
			}
		}
		Reserved.AppendTags(Domain.States);
	}
	Reason.Reset();
	return true;
}
