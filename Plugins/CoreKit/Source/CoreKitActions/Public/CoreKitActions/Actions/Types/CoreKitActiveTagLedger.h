#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Tracks idempotent gameplay-tag claims by source.
 * A tag remains active until every source that claimed it releases it.
 */
class COREKITACTIONS_API FCoreKitActiveTagLedger
{
public:
	bool Add(FGameplayTag Tag, const void* Source);
	bool Remove(FGameplayTag Tag, const void* Source);
	bool Clear(FGameplayTag Tag);
	void RemoveAllFromSource(const void* Source);
	void Reset();

	bool Has(FGameplayTag Tag) const;
	bool HasExact(FGameplayTag Tag) const;
	bool HasSource(FGameplayTag Tag, const void* Source) const;
	const FGameplayTagContainer& GetTags() const { return ActiveTags; }

private:
	void RefreshTag(FGameplayTag Tag);

	TMap<FGameplayTag, TSet<const void*>> TagSources;
	FGameplayTagContainer ActiveTags;
};
