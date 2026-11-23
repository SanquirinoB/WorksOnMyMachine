#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "CoreKitActions/Actions/Interfaces/CoreKitActionSource.h"
#include "CoreKitActionsLog.h"
#include "GameFramework/Actor.h"
#include "CoreKitActions/States/CoreKitStateComponent.h"

void UCoreKitActionComponent::AddLooseTag(const FGameplayTag Tag)
{
	if (bEvaluatingActionPolicy) { return; }
	AddActiveTagFromSource(Tag, this);
}

void UCoreKitActionComponent::RemoveLooseTag(const FGameplayTag Tag)
{
	if (bEvaluatingActionPolicy) { return; }
	RemoveActiveTagFromSource(Tag, this);
}

void UCoreKitActionComponent::ClearActiveTag(const FGameplayTag Tag)
{
	if (bEvaluatingActionPolicy) { return; }
	if (IsReservedStateWrite(Tag))
	{
		return;
	}
	// Never erase another owner's claim through this legacy entry point.
	RemoveActiveTagFromSource(Tag, this);
}

void UCoreKitActionComponent::AddActiveTagFromSource(const FGameplayTag Tag, const void* Source)
{
	if (IsReservedStateWrite(Tag))
	{
		return;
	}
	const FGameplayTagContainer PreviousTags = ActiveTags;
	if (!ActiveTagLedger.Add(Tag, Source))
	{
		return;
	}

	RefreshActiveTags();
	BroadcastTagChanges(PreviousTags);
	TryActivateQueuedAction();
}

bool UCoreKitActionComponent::RemoveActiveTagFromSource(const FGameplayTag Tag, const void* Source)
{
	if (IsReservedStateWrite(Tag))
	{
		return false;
	}
	const FGameplayTagContainer PreviousTags = ActiveTags;
	if (!ActiveTagLedger.Remove(Tag, Source))
	{
		return false;
	}

	RefreshActiveTags();
	BroadcastTagChanges(PreviousTags);
	TryActivateQueuedAction();
	return true;
}

void UCoreKitActionComponent::RemoveAllActiveTagsFromSource(const void* Source)
{
	const FGameplayTagContainer PreviousTags = ActiveTags;
	ActiveTagLedger.RemoveAllFromSource(Source);
	RefreshActiveTags();
	BroadcastTagChanges(PreviousTags);
	TryActivateQueuedAction();
}

bool UCoreKitActionComponent::HasActiveTagFromSource(const FGameplayTag Tag, const void* Source) const
{
	return ActiveTagLedger.HasSource(Tag, Source);
}

bool UCoreKitActionComponent::HasActiveTag(const FGameplayTag Tag) const
{
	return ActiveTags.HasTag(Tag);
}

bool UCoreKitActionComponent::HasAnyActiveTags(const FGameplayTagContainer& Tags) const
{
	return ActiveTags.HasAny(Tags);
}

bool UCoreKitActionComponent::HasAllActiveTags(const FGameplayTagContainer& Tags) const
{
	return ActiveTags.HasAll(Tags);
}

void UCoreKitActionComponent::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = ActiveTags;
}

void UCoreKitActionComponent::ApplyEndTagChanges(const UCoreKitActionDefinition& Definition)
{
	for (const FGameplayTag& TagToAdd : Definition.TagsToAddOnEnd)
	{
		AddLooseTag(TagToAdd);
	}

	for (const FGameplayTag& TagToClear : Definition.TagsToClearOnEnd)
	{
		ClearActiveTag(TagToClear);
	}
}

void UCoreKitActionComponent::BroadcastTagChanges(const FGameplayTagContainer& PreviousTags)
{
	// Delegates may synchronously write claims. Compute both notifications before invoking them so
	// neither loop observes a container mutated by an earlier listener.
	TArray<FGameplayTag> Added;
	TArray<FGameplayTag> Removed;
	for (const FGameplayTag& Tag : ActiveTags)
	{
		if (!PreviousTags.HasTagExact(Tag))
		{
			Added.Add(Tag);
		}
	}

	for (const FGameplayTag& Tag : PreviousTags)
	{
		if (!ActiveTags.HasTagExact(Tag))
		{
			Removed.Add(Tag);
		}
	}
	Added.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
	Removed.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
	for (const FGameplayTag& Tag : Added) { OnActiveTagAdded.Broadcast(Tag); }
	for (const FGameplayTag& Tag : Removed) { OnActiveTagRemoved.Broadcast(Tag); }
}

void UCoreKitActionComponent::RefreshActiveTags()
{
	ActiveTags = ActiveTagLedger.GetTags();
	if (IsValid(StateComponent))
	{
		FGameplayTagContainer StateTags;
		StateComponent->GetOwnedGameplayTags(StateTags);
		ActiveTags.AppendTags(StateTags);
	}
	ActiveTags.RemoveTag(CoreKitActionTags::IASA);
	if (IsIASA())
	{
		ActiveTags.AddTag(CoreKitActionTags::IASA);
	}
}

void UCoreKitActionComponent::SetTagClaim(UObject* Source, const FGameplayTag Tag, const bool bActive)
{
	if (bEvaluatingActionPolicy || !IsValid(Source) || !Tag.IsValid())
	{
		return;
	}
	if (IsReservedStateWrite(Tag))
	{
		return;
	}
	if (bActive)
	{
		ObjectClaimSources.Add(Source, Source);
		AddActiveTagFromSource(Tag, Source);
	}
	else
	{
		RemoveActiveTagFromSource(Tag, Source);
	}
}

void UCoreKitActionComponent::ReleaseTagClaims(UObject* Source)
{
	if (!Source)
	{
		return;
	}
	RemoveAllActiveTagsFromSource(Source);
	ObjectClaimSources.Remove(Source);
}

void UCoreKitActionComponent::PruneObjectClaims()
{
	{
		TGuardValue<int32> Guard(MutationDepth, MutationDepth + 1);
		TArray<const void*> DeadSources;
		for (const auto& Pair : ObjectClaimSources)
		{
			if (!Pair.Value.IsValid())
			{
				DeadSources.Add(Pair.Key);
			}
		}
		for (const auto* Source : DeadSources)
		{
			RemoveAllActiveTagsFromSource(Source);
			ObjectClaimSources.Remove(Source);
		}
	}
	RefreshIASA();
}

TArray<FString> UCoreKitActionComponent::GetTagSources(const FGameplayTag Tag) const
{
	TArray<FString> Result;
	if (IsValid(StateComponent))
	{
		Result.Append(StateComponent->GetStateSources(Tag));
	}
	for (const auto& ActiveTag : ActiveTags)
	{
		if (!ActiveTag.MatchesTag(Tag))
		{
			continue;
		}
		for (const auto& Pair : ObjectClaimSources)
		{
			if (ActiveTagLedger.HasSource(ActiveTag, Pair.Key))
			{
				Result.AddUnique(
					FString::Printf(TEXT("%s: %s"), *ActiveTag.ToString(), *GetPathNameSafe(Pair.Value.Get())));
			}
		}
		if (ActiveTagLedger.HasSource(ActiveTag, this))
		{
			Result.AddUnique(FString::Printf(TEXT("%s: %s (loose)"), *ActiveTag.ToString(), *GetPathName()));
		}
	}
	if (Result.IsEmpty() && HasActiveTag(Tag))
	{
		Result.Add(TEXT("Native source (use SetTagClaim for named diagnostics)"));
	}
	Result.Sort();
	return Result;
}

bool UCoreKitActionComponent::CanReserveStateTags(const FGameplayTagContainer& Tags, FString& Reason) const
{
	if (Tags.HasTagExact(CoreKitActionTags::IASA))
	{
		Reason = TEXT("State.Action.IASA is derived by the action coordinator and cannot be owned by a state domain.");
		return false;
	}
	FGameplayTagContainer Claims = ActiveTagLedger.GetTags();
	if (IsValid(ActionSet))
	{
		Claims.AppendTags(ActionSet->InitialTags);
	}
	for (const auto& Reserved : Tags)
	{
		for (const auto& Claim : Claims)
		{
			if (Claim.MatchesTag(Reserved) || Reserved.MatchesTag(Claim))
			{
				Reason = FString::Printf(TEXT("Legacy claim/InitialTag %s conflicts with reserved state %s. Migrate "
					"its writer before cutover."),
				                         *Claim.ToString(), *Reserved.ToString());
				return false;
			}
		}
	}
	Reason.Reset();
	return true;
}

bool UCoreKitActionComponent::AttachStateComponent(UCoreKitStateComponent* States)
{
	if (!IsValid(States) || States->GetOwner() != GetOwner() || bEndingPlay)
	{
		return false;
	}
	if (StateComponent)
	{
		return StateComponent == States;
	}
	StateComponent = States;
	bStateRuntimeRequired = true;
	States->OnStatesSettled.AddUObject(this, &UCoreKitActionComponent::HandleStatesSettled);
	RefreshAuthoritativeStates();
	return true;
}

void UCoreKitActionComponent::RefreshAuthoritativeStates()
{
	if (bEndingPlay)
	{
		return;
	}
	TGuardValue<int32> Guard(MutationDepth, MutationDepth + 1);
	const auto Previous = ActiveTags;
	RefreshActiveTags();
	BroadcastTagChanges(Previous);
}

void UCoreKitActionComponent::HandleStatesSettled()
{
	if (!bEndingPlay)
	{
		TryActivateQueuedAction();
	}
}

bool UCoreKitActionComponent::IsReservedStateWrite(const FGameplayTag Tag) const
{
	if (Tag == CoreKitActionTags::IASA)
	{
		return true;
	}
	if (IsValid(StateComponent) && StateComponent->IsTagReserved(Tag))
	{
		UE_LOG(LogCoreKitActions, Warning,
		       TEXT("Rejected direct write to reserved state %s on %s; use its domain authority."), *Tag.ToString(),
		       *GetNameSafe(GetOwner()));
		return true;
	}
	return false;
}
