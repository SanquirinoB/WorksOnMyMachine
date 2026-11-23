#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "CoreKitActions/Actions/Interfaces/CoreKitActionSource.h"
#include "CoreKitActionsLog.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CoreKitActions/States/CoreKitStateComponent.h"

FCoreKitActionRequestReport UCoreKitActionComponent::EvaluatePolicy(const FGameplayTag ActionTag,
	const FGameplayTag InputTag, const bool bInternalCommitCheck, const bool bCleanup) const
{
	FCoreKitActionRequestReport Report;
	Report.ActionTag = ActionTag;
	Report.InputTag = InputTag;
	Report.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Report.StateRevision = IsValid(StateComponent) ? StateComponent->GetRevision() : 0;
	if ((!bInternalCommitCheck && MutationDepth > 0) ||
		(IsValid(StateComponent) && StateComponent->IsCommittingTransition()))
	{
		Report.Result = ECoreKitActionRequestResult::TransitionInProgress;
		Report.FailureCode = TEXT("CoordinatorTransition");
		Report.Detail = TEXT("A start/end callback is still committing its transition.");
		return Report;
	}
	if (bStateRuntimeRequired && !bCleanup)
	{
		FString Reason;
		if (!IsValid(StateComponent) || !StateComponent->IsHealthy(Reason))
		{
			Report.FailureCode = TEXT("StateAuthorityUnavailable");
			Report.Detail = Reason.IsEmpty() ? TEXT("Required state runtime was destroyed.") : Reason;
			return Report;
		}
	}
	if (bEndingPlay || !IsValid(GetOwner()) || !IsValid(ActionSet) || !ActionTag.IsValid())
	{
		Report.FailureCode = TEXT("InvalidActionConfiguration");
		Report.Detail = TEXT("An owner, Action Set and valid action tag are required; teardown rejects new requests.");
		return Report;
	}
	bool bPersistent = false;
	const auto* Entry = FindActionEntry(ActionTag, bPersistent);
	if (!Entry)
	{
		Report.Result = ECoreKitActionRequestResult::ActionNotFound;
		Report.FailureCode = TEXT("ActionNotRegistered");
		return Report;
	}
	if (!IsValid(Entry->Definition) || !IsValid(Entry->ActionTemplate) ||
		Entry->ActionTemplate->GetClass()->HasAnyClassFlags(CLASS_Abstract))
	{
		Report.FailureCode = TEXT("MissingActionTemplateOrDefinition");
		return Report;
	}
	const auto* Definition = Entry->Definition.Get();
	if (Entry->ActionTemplate->IsA<UCoreKitCommandAction>() &&
		(bPersistent || !UCoreKitCommandAction::ValidatePolicy(Definition, Report.Detail)))
	{
		Report.FailureCode = TEXT("InvalidCommandPolicy");
		return Report;
	}
	if (bCleanup)
	{
		Report.Result = ECoreKitActionRequestResult::Activated;
		Report.Status = ECoreKitActionRequestStatus::Allowed;
		return Report;
	}
	if (!bPersistent && Definition->bRequiresIASA && !IsIASA())
	{
		Report.Result = ECoreKitActionRequestResult::BlockedByTags;
		Report.FailureCode = TEXT("IASAClosed");
		Report.Detail = TEXT("The actor is not ready for voluntary gameplay.");
		return Report;
	}
	FGameplayTagContainer Writes = Definition->GrantedTagsWhileActive;
	Writes.AppendTags(Definition->TagsToClearOnStart);
	Writes.AppendTags(Definition->TagsToAddOnEnd);
	Writes.AppendTags(Definition->TagsToClearOnEnd);
	for (const auto& Tag : Writes)
	{
		if (Tag == CoreKitActionTags::IASA || (IsValid(StateComponent) && StateComponent->IsTagReserved(Tag)))
		{
			Report.FailureCode = TEXT("ReservedStateMutation");
			Report.Detail = FString::Printf(
				TEXT("ActionDefinition cannot directly modify authoritative state '%s'. Request a domain transition."),
				*Tag.ToString());
			return Report;
		}
	}
	if (const auto* Existing = GetActiveAction(ActionTag); IsValid(Existing))
	{
		Report.ExecutionId = Existing->GetExecutionId();
		if (Existing->IsEnding())
		{
			Report.Result = ECoreKitActionRequestResult::TransitionInProgress;
		Report.FailureCode = TEXT("WaitingForActionCleanup");
			return Report;
		}
		if (Definition->RepeatPolicy == ECoreKitActionRepeatPolicy::Reject)
		{
			Report.Result = ECoreKitActionRequestResult::AlreadyActive;
			Report.FailureCode = TEXT("ActionAlreadyActive");
			return Report;
		}
		if (!CanCancelActionTag(ActionTag, false))
		{
			Report.ConflictingActions.AddTag(ActionTag);
		}
	}
	for (const auto& Pair : ActiveActions)
	{
		if (Pair.Key != ActionTag && IsValid(Pair.Value) &&
			(Definition->bCancelOtherActions || Pair.Key.MatchesAny(Definition->ActionsToCancel)) &&
			!CanCancelActionTag(Pair.Key, false))
		{
			Report.ConflictingActions.AddTag(Pair.Key);
		}
	}
	for (const auto& Tag : Definition->RequiredTags)
	{
		if (!ActiveTags.HasTag(Tag))
		{
			Report.MissingTags.AddTag(Tag);
		}
	}
	for (const auto& Tag : ActiveTags)
	{
		if (Tag.MatchesAny(Definition->BlockedTags))
		{
			Report.BlockingTags.AddTag(Tag);
			for (const auto& Source : GetTagSources(Tag))
			{
				Report.BlockingSources.AddUnique(Source);
			}
		}
	}
	if (!Report.ConflictingActions.IsEmpty())
	{
		Report.Result = ECoreKitActionRequestResult::BlockedByNonCancelableAction;
		Report.FailureCode = TEXT("NonCancelableConflict");
	}
	else if (!Report.MissingTags.IsEmpty())
	{
		Report.Result = ECoreKitActionRequestResult::MissingRequirements;
		Report.FailureCode = TEXT("MissingRequiredTags");
	}
	else if (!Report.BlockingTags.IsEmpty())
	{
		Report.Result = ECoreKitActionRequestResult::BlockedByTags;
		Report.FailureCode = TEXT("BlockedByStateTags");
	}
	else
	{
		Report.Result = ECoreKitActionRequestResult::Activated;
		Report.Status = ECoreKitActionRequestStatus::Allowed;
	}
	return Report;
}

FCoreKitActionRequestReport UCoreKitActionComponent::EvaluateAction(
	const FGameplayTag ActionTag, const FGameplayTag ActivationInputTag) const
{
	TGuardValue<int32> EvaluationGuard(const_cast<UCoreKitActionComponent*>(this)->MutationDepth,
		const_cast<UCoreKitActionComponent*>(this)->MutationDepth + 1);
	TGuardValue<bool> PolicyGuard(const_cast<UCoreKitActionComponent*>(this)->bEvaluatingActionPolicy, true);
	auto Report = EvaluatePolicy(ActionTag, ActivationInputTag, true);
	if (Report.Result != ECoreKitActionRequestResult::Activated)
	{
		return Report;
	}
	bool bPersistent = false;
	const auto* Entry = FindActionEntry(ActionTag, bPersistent);
	if (Entry->ActionTemplate->IsA<UCoreKitCommandAction>())
	{
		Report.Result = ECoreKitActionRequestResult::RejectedByAction;
		Report.FailureCode = TEXT("CommandRequiresTypedDispatch");
		return Report;
	}
	// Query instances are never registered with GameplayTasks and never activated.
	auto* Probe = NewObject<UCoreKitAction>(
		GetTransientPackageAsObject(), Entry->ActionTemplate->GetClass(), NAME_None, RF_Transient,
		Entry->ActionTemplate);
	Probe->InitializeAction(
		const_cast<UCoreKitActionComponent*>(this), ActionTag, ActivationInputTag, Entry->Definition);
	if (!Probe->EvaluateActivation(Report.FailureCode, Report.Detail))
	{
		Report.Result = ECoreKitActionRequestResult::RejectedByAction;
	}
	NormalizeReport(Report);
	return Report;
}

FCoreKitActionRequestReport UCoreKitActionComponent::EvaluateCommandPolicy(
	FGameplayTag Tag, const UActorComponent* Source) const
{
	auto Report = EvaluatePolicy(Tag, FGameplayTag());
	bool bPersistent = false;
	const auto* Entry = FindActionEntry(Tag, bPersistent);
	if (!IsValid(Source) || Source->GetOwner() != GetOwner())
	{
		Report.Result = ECoreKitActionRequestResult::InvalidConfiguration;
		Report.FailureCode = TEXT("InvalidCommandSource");
	}
	else if (Entry && IsValid(Entry->ActionTemplate) && !Entry->ActionTemplate->IsA<UCoreKitCommandAction>())
	{
		Report.Result = ECoreKitActionRequestResult::InvalidConfiguration;
		Report.FailureCode = TEXT("CommandTemplateRequired");
	}
	NormalizeReport(Report);
	return Report;
}

FCoreKitActionRequestReport UCoreKitActionComponent::ExecuteActionCommand(FGameplayTag Tag,
	FGameplayTag Input, UActorComponent* Source, TFunctionRef<bool(FName&, FString&)> Readiness,
	TFunctionRef<bool(FName&, FString&)> Dispatch, bool bCleanup, bool bContinuous, bool bAllowNestedActionRequests)
{
	PruneObjectClaims();
	auto Report = EvaluatePolicy(Tag, Input, false, bCleanup);
	Report.RequestId = FGuid::NewGuid();
	bool bPersistent = false;
	const auto* Entry = FindActionEntry(Tag, bPersistent);
	if (!IsValid(Source) || Source->GetOwner() != GetOwner() || !Entry || !IsValid(Entry->ActionTemplate) ||
		!Entry->ActionTemplate->IsA<UCoreKitCommandAction>())
	{
		Report.Result = ECoreKitActionRequestResult::InvalidConfiguration;
		Report.FailureCode = TEXT("InvalidCommandSourceOrTemplate");
	}
	if (Report.Result == ECoreKitActionRequestResult::Activated)
	{
		const FCoreKitActionEntry EntrySnapshot = *Entry;
		TGuardValue<int32> Guard(MutationDepth, MutationDepth + 1);
		if (!Readiness(Report.FailureCode, Report.Detail))
		{
			Report.Result = ECoreKitActionRequestResult::RejectedByAction;
			if (Report.FailureCode.IsNone())
			{
				Report.FailureCode = TEXT("CommandReadinessRejected");
			}
		}
		else
		{
			// Defense against a misbehaving readiness callback that publishes a restriction.
			Report = EvaluatePolicy(Tag, Input, true, bCleanup);
			const auto* CurrentEntry = FindActionEntry(Tag, bPersistent);
			if (!IsValid(Source) || Source->GetOwner() != GetOwner() || !CurrentEntry ||
				CurrentEntry->Definition != EntrySnapshot.Definition ||
				CurrentEntry->ActionTemplate != EntrySnapshot.ActionTemplate)
			{
				Report.Result = ECoreKitActionRequestResult::InvalidConfiguration;
				Report.FailureCode = TEXT("CommandConfigurationChangedDuringReadiness");
			}
			if (Report.Result == ECoreKitActionRequestResult::Activated)
			{
				if (!bContinuous)
				{
					Report.ExecutionId = FGuid::NewGuid();
				}
				TGuardValue<int32> DispatchGuard(
					MutationDepth, bAllowNestedActionRequests ? MutationDepth - 1 : MutationDepth);
				if (!Dispatch(Report.FailureCode, Report.Detail))
				{
					Report.Result = ECoreKitActionRequestResult::RejectedByAction;
					if (Report.FailureCode.IsNone())
					{
						Report.FailureCode = TEXT("CommandBackendRejected");
					}
				}
				else
				{
					Report.Status = ECoreKitActionRequestStatus::Executed;
				}
			}
		}
	}
	Report.Status = Report.Result == ECoreKitActionRequestResult::Activated
		? ECoreKitActionRequestStatus::Executed
		: ECoreKitActionRequestStatus::Rejected;
	const auto* Previous = bContinuous ? ContinuousCommandReports.Find(Tag) : nullptr;
	const bool bChanged =
		!Previous || Previous->Result != Report.Result || Previous->FailureCode != Report.FailureCode ||
		Previous->BlockingTags != Report.BlockingTags || Previous->MissingTags != Report.MissingTags ||
		Previous->BlockingSources != Report.BlockingSources;
	if (bContinuous)
	{
		ContinuousCommandReports.Add(Tag, Report);
	}
	if (bChanged)
	{
		RecordRequest(Report);
	}
	else
	{
		LastRequestReport = Report;
	}
	TryActivateQueuedAction();
	return Report;
}

void UCoreKitActionComponent::NormalizeReport(FCoreKitActionRequestReport& Report) const
{
	if (Report.Result == ECoreKitActionRequestResult::Queued)
	{
		Report.Status = ECoreKitActionRequestStatus::Queued;
	}
	else if (Report.Result != ECoreKitActionRequestResult::Activated)
	{
		Report.Status = ECoreKitActionRequestStatus::Rejected;
	}
	if (Report.FailureCode.IsNone() && Report.Result != ECoreKitActionRequestResult::Activated)
	{
		Report.FailureCode = FName(
			*StaticEnum<ECoreKitActionRequestResult>()->GetNameStringByValue(static_cast<int64>(Report.Result)));
	}
}

void UCoreKitActionComponent::RecordRequest(FCoreKitActionRequestReport& Report)
{
	NormalizeReport(Report);
	LastRequestReport = Report;
	const int32 Limit = FMath::Clamp(RequestHistoryLimit, 0, 256);
	if (Limit > 0)
	{
		RequestHistory.Add(Report);
		if (RequestHistory.Num() > Limit)
		{
			RequestHistory.RemoveAt(0, RequestHistory.Num() - Limit);
		}
	}
	else
	{
		RequestHistory.Reset();
	}
	if (bLogActionRequests)
	{
		UE_LOG(LogCoreKitActions, Display,
		       TEXT("Request %s owner=%s result=%s reason=%s missing=[%s] blocking=[%s] sources=[%s] %s"),
		       *Report.ActionTag.ToString(), *GetNameSafe(GetOwner()),
		       *StaticEnum<ECoreKitActionRequestResult>()->GetNameStringByValue(static_cast<int64>(Report.Result)
		       ),
		       *Report.FailureCode.ToString(), *Report.MissingTags.ToStringSimple(),
		       *Report.BlockingTags.ToStringSimple(),
		       *FString::Join(Report.BlockingSources, TEXT(", ")), *Report.Detail);
	}
	// A diagnostics listener requesting another action must not recursively notify itself.
	thread_local bool bNotifying = false;
	if (!bNotifying)
	{
		TGuardValue<bool> Guard(bNotifying, true);
		OnActionRequestEvaluated.Broadcast(Report);
	}
}
