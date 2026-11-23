#include "CoreKitActions/Actions/Types/CoreKitActiveTagLedger.h"

bool FCoreKitActiveTagLedger::Add(const FGameplayTag Tag, const void* Source)
{
	if (!Tag.IsValid() || Source == nullptr)
	{
		return false;
	}

	TSet<const void*>& Sources = TagSources.FindOrAdd(Tag);
	const bool bAdded = !Sources.Contains(Source);
	Sources.Add(Source);
	RefreshTag(Tag);
	return bAdded;
}

bool FCoreKitActiveTagLedger::Remove(const FGameplayTag Tag, const void* Source)
{
	if (Source == nullptr)
	{
		return false;
	}

	TSet<const void*>* Sources = TagSources.Find(Tag);
	if (Sources == nullptr)
	{
		return false;
	}

	const bool bRemoved = Sources->Remove(Source) > 0;
	if (Sources->IsEmpty())
	{
		TagSources.Remove(Tag);
	}

	RefreshTag(Tag);
	return bRemoved;
}

bool FCoreKitActiveTagLedger::Clear(const FGameplayTag Tag)
{
	const bool bWasActive = ActiveTags.HasTagExact(Tag);
	TagSources.Remove(Tag);
	ActiveTags.RemoveTag(Tag);
	return bWasActive;
}

void FCoreKitActiveTagLedger::RemoveAllFromSource(const void* Source)
{
	if (Source == nullptr)
	{
		return;
	}

	TArray<FGameplayTag> Tags;
	TagSources.GetKeys(Tags);

	for (const FGameplayTag& Tag : Tags)
	{
		Remove(Tag, Source);
	}
}

void FCoreKitActiveTagLedger::Reset()
{
	TagSources.Reset();
	ActiveTags.Reset();
}

bool FCoreKitActiveTagLedger::Has(const FGameplayTag Tag) const
{
	return ActiveTags.HasTag(Tag);
}

bool FCoreKitActiveTagLedger::HasExact(const FGameplayTag Tag) const
{
	return ActiveTags.HasTagExact(Tag);
}

bool FCoreKitActiveTagLedger::HasSource(const FGameplayTag Tag, const void* Source) const
{
	const TSet<const void*>* Sources = TagSources.Find(Tag);
	return Source != nullptr && Sources != nullptr && Sources->Contains(Source);
}

void FCoreKitActiveTagLedger::RefreshTag(const FGameplayTag Tag)
{
	const TSet<const void*>* Sources = TagSources.Find(Tag);
	if (Sources != nullptr && !Sources->IsEmpty())
	{
		ActiveTags.AddTag(Tag);
	}
	else
	{
		ActiveTags.RemoveTag(Tag);
	}
}
