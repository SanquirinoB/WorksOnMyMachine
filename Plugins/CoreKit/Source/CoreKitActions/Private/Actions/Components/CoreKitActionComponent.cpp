#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"

#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitActions/Actions/CoreKitActionContext.h"
#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "CoreKitActions/Actions/Interfaces/CoreKitActionSource.h"
#include "CoreKitActionsLog.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "CoreKitActions/States/CoreKitStateComponent.h"
#include "TimerManager.h"

UCoreKitActionComponent::UCoreKitActionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// GameplayTasksComponent enables this tick only while at least one task opts in.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCoreKitActionComponent::BeginPlay()
{
	// UActorComponent::BeginPlay dispatches Blueprint ReceiveBeginPlay. Prepare the context first
	// so sibling components and Blueprint logic can safely request actions during startup.
	RefreshActorContext();
	RefreshIASA();
	ObserveIASAAnimations();
	Super::BeginPlay();
	if (GetOwner())
	{
		if (auto* States = GetOwner()->FindComponentByClass<UCoreKitStateComponent>())
		{
			States->BindActions(this);
		}
	}

	ActionSet = ResolveActionSet();
	if (!IsValid(ActionSet))
	{
		UE_LOG(LogCoreKitActions, Warning,
		       TEXT("CoreKitActionComponent '%s' on owner '%s' has no Action Set; action requests will be "
			       "rejected. Check for an unconfigured or duplicate coordinator."),
		       *GetPathName(), *GetPathNameSafe(GetOwner()));
		return;
	}

	for (const FGameplayTag& InitialTag : ActionSet->InitialTags)
	{
		AddLooseTag(InitialTag);
	}

	bInitialized = true;

	for (const TPair<FGameplayTag, FCoreKitActionEntry>& Pair : ActionSet->PersistentActions)
	{
		const ECoreKitActionRequestResult Result = RequestActionInternal(
			Pair.Key, FGameplayTag(), false, true, nullptr, FGuid::NewGuid()).Result;

		if (Result != ECoreKitActionRequestResult::Activated)
		{
			UE_LOG(LogCoreKitActions, Warning,
			       TEXT("Persistent action '%s' failed to start on '%s' (result %d)."), *Pair.Key.ToString(),
			       *GetNameSafe(GetOwner()), static_cast<int32>(Result));
		}
	}
}

void UCoreKitActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	bInitialized = false;
	UnbindIASAAnimations();
	NotifyComponentEnding();
	ClearQueuedAction();

	TArray<TObjectPtr<UCoreKitAction>> ActionsToEnd;
	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : ActiveActions)
	{
		if (IsValid(Pair.Value))
		{
			ActionsToEnd.AddUnique(Pair.Value);
		}
	}

	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : PersistentActions)
	{
		if (IsValid(Pair.Value))
		{
			ActionsToEnd.AddUnique(Pair.Value);
		}
	}

	for (UCoreKitAction* Action : ActionsToEnd)
	{
		Action->FinishAction(ECoreKitActionEndReason::OwnerEndPlay);
	}

	ActiveActions.Reset();
	PersistentActions.Reset();
	CurrentAction = nullptr;
	LastAction = nullptr;
	ActiveTagLedger.Reset();
	ActiveTags.Reset();
	ObjectClaimSources.Reset();
	IASABlocks.Reset();
	bIASA = false;
	ContinuousCommandReports.Reset();
	ActorContext.Reset();
	if (IsValid(StateComponent))
	{
		StateComponent->OnStatesSettled.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UCoreKitActionComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	bEndingPlay = true;
	UnbindIASAAnimations();
	NotifyComponentEnding();
	ClearQueuedAction();
	ActorContext.Reset();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UCoreKitActionComponent::NotifyComponentEnding()
{
	if (!bComponentEndingNotified)
	{
		bComponentEndingNotified = true;
		OnActionComponentEnding.Broadcast();
	}
}

ECoreKitActionRequestResult UCoreKitActionComponent::RequestAction(
	const FGameplayTag ActionTag, const FGameplayTag ActivationInputTag)
{
	return RequestActionWithContext(ActionTag, ActivationInputTag, nullptr).Result;
}

FCoreKitActionRequestReport UCoreKitActionComponent::RequestActionWithContext(
	const FGameplayTag ActionTag, const FGameplayTag ActivationInputTag, UCoreKitActionContext* Context)
{
	bool bPersistent = false;
	FindActionEntry(ActionTag, bPersistent);
	const FGuid RequestId = FGuid::NewGuid();
	return RequestActionInternal(ActionTag, ActivationInputTag, true, bPersistent, Context, RequestId);
}

bool UCoreKitActionComponent::TryActivateAction(
	const FGameplayTag ActionTag, const FGameplayTag ActivationInputTag)
{
	return RequestAction(ActionTag, ActivationInputTag) == ECoreKitActionRequestResult::Activated;
}

ECoreKitActionRequestResult UCoreKitActionComponent::HandleInputTag(const FGameplayTag InputTag)
{
	if (!IsValid(ActionSet) || !InputTag.IsValid())
	{
		return ECoreKitActionRequestResult::ActionNotFound;
	}

	struct FCandidate
	{
		FGameplayTag ActionTag;
		int32 Priority = 0;
	};

	TArray<FCandidate> Candidates;
	for (const TPair<FGameplayTag, FCoreKitActionEntry>& Pair : ActionSet->Actions)
	{
		const UCoreKitActionDefinition* Definition = Pair.Value.Definition;
		if (IsValid(Definition) && Definition->ActivationInputTags.HasTag(InputTag))
		{
			Candidates.Add({Pair.Key, Definition->SelectionPriority});
		}
	}

	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}

		return A.ActionTag.ToString() < B.ActionTag.ToString();
	});

	ECoreKitActionRequestResult LastResult = ECoreKitActionRequestResult::ActionNotFound;
	for (const FCandidate& Candidate : Candidates)
	{
		LastResult = RequestActionInternal(
			Candidate.ActionTag, InputTag, true, false, nullptr, FGuid::NewGuid()).Result;
		if (LastResult == ECoreKitActionRequestResult::Activated ||
			LastResult == ECoreKitActionRequestResult::Queued)
		{
			return LastResult;
		}
	}

	return LastResult;
}

FCoreKitActionRequestReport UCoreKitActionComponent::RequestActionInternal(
	const FGameplayTag ActionTag, const FGameplayTag ActivationInputTag, const bool bAllowQueue,
	const bool bPersistent, UCoreKitActionContext* Context, const FGuid RequestId)
{
	PruneObjectClaims();
	FCoreKitActionRequestReport Report = EvaluatePolicy(ActionTag, ActivationInputTag);
	Report.RequestId = RequestId.IsValid() ? RequestId : FGuid::NewGuid();
	if (Context != nullptr && (!IsValid(Context) || !Context->IsActionContextValid()))
	{
		Report.Result = ECoreKitActionRequestResult::InvalidContext;
		Report.Status = ECoreKitActionRequestStatus::Rejected;
		Report.FailureCode = TEXT("InvalidActionContext");
		Report.Detail = TEXT("The request context is no longer valid.");
	}
	bool bResolvedPersistent = false;
	const FCoreKitActionEntry* Entry = FindActionEntry(ActionTag, bResolvedPersistent);
	if (Report.Result == ECoreKitActionRequestResult::Activated && Entry)
	{
		if (Entry->ActionTemplate->IsA<UCoreKitCommandAction>())
		{
			Report.Result = ECoreKitActionRequestResult::RejectedByAction;
			Report.FailureCode = TEXT("CommandRequiresTypedDispatch");
			RecordRequest(Report);
			return Report;
		}
		const FCoreKitActionEntry EntrySnapshot = *Entry;
		Report.Result = StartAction(
			ActionTag, ActivationInputTag, EntrySnapshot, bResolvedPersistent, Context, Report);
		Report.Status = Report.Result == ECoreKitActionRequestResult::Activated
			? ECoreKitActionRequestStatus::Started
			: ECoreKitActionRequestStatus::Rejected;
		Entry = FindActionEntry(ActionTag, bResolvedPersistent);
	}
	const bool bRetryable = Report.Result == ECoreKitActionRequestResult::MissingRequirements ||
		Report.Result == ECoreKitActionRequestResult::BlockedByTags ||
		Report.Result == ECoreKitActionRequestResult::RejectedByAction ||
		Report.Result == ECoreKitActionRequestResult::TransitionInProgress;
	if (bRetryable && bAllowQueue && MutationDepth == 0 && !bResolvedPersistent && Entry &&
		IsValid(Entry->Definition) && Entry->Definition->bQueueWhenBlocked)
	{
		if (QueueAction(ActionTag, ActivationInputTag, Context, Report.RequestId,
			Entry->Definition->QueuePriority, Entry->Definition->QueueWindowSeconds))
		{
			Report.Result = ECoreKitActionRequestResult::Queued;
			Report.Status = ECoreKitActionRequestStatus::Queued;
		}
		else
		{
			Report.Result = ECoreKitActionRequestResult::QueueRejected;
			Report.Status = ECoreKitActionRequestStatus::Rejected;
			Report.FailureCode = TEXT("QueuePriorityRejected");
			Report.Detail = TEXT("A higher-priority request already owns the queue slot.");
		}
	}
	RecordRequest(Report);
	return Report;
}

ECoreKitActionRequestResult UCoreKitActionComponent::StartAction(const FGameplayTag ActionTag,
                                                                               const FGameplayTag ActivationInputTag,
                                                                               const FCoreKitActionEntry& Entry,
																									   const bool bPersistent,
																									   UCoreKitActionContext* Context,
																									   FCoreKitActionRequestReport&
                                                                               Report)
{
	UCoreKitActionDefinition* Definition = Entry.Definition;
	if (!IsValid(Entry.ActionTemplate) || !IsValid(Definition))
	{
		return ECoreKitActionRequestResult::InvalidConfiguration;
	}

	if ((ActiveActions.Contains(ActionTag) || PersistentActions.Contains(ActionTag)) &&
		!CanCancelActionTag(ActionTag, false))
	{
		return ECoreKitActionRequestResult::BlockedByNonCancelableAction;
	}

	UCoreKitAction* NewAction =
		UCoreKitAction::CreateAction(*this, this, Entry.ActionTemplate->GetClass(), Entry.ActionTemplate);
	if (!IsValid(NewAction))
	{
		return ECoreKitActionRequestResult::InvalidConfiguration;
	}

	NewAction->InitializeAction(this, ActionTag, ActivationInputTag, Definition, Context);
	FName FailureCode;
	FString FailureDetail;
	{
		TGuardValue<int32> EvaluationGuard(MutationDepth, MutationDepth + 1);
		TGuardValue<bool> PolicyGuard(bEvaluatingActionPolicy, true);
		if (!NewAction->EvaluateActivation(FailureCode, FailureDetail))
		{
			Report.FailureCode = FailureCode;
			Report.Detail = FailureDetail;
			return ECoreKitActionRequestResult::RejectedByAction;
		}
	}

	{
		TGuardValue<int32> MutationGuard(MutationDepth, MutationDepth + 1);
		if (UCoreKitAction* ExistingAction = GetActiveAction(ActionTag))
		{
			ExistingAction->RequestCancellation(ECoreKitActionEndReason::Replaced);
			if (GetActiveAction(ActionTag))
			{
				return ECoreKitActionRequestResult::TransitionInProgress;
			}
		}

		TArray<TObjectPtr<UCoreKitAction>> ActionsToCancel;
		for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : ActiveActions)
		{
			const bool bExplicitlyCancelled = Pair.Key.MatchesAny(Definition->ActionsToCancel);
			if ((Definition->bCancelOtherActions || bExplicitlyCancelled) && CanCancelActionTag(Pair.Key, false) &&
				IsValid(Pair.Value))
			{
				ActionsToCancel.AddUnique(Pair.Value);
			}
		}

		for (UCoreKitAction* ActionToCancel : ActionsToCancel)
		{
			ActionToCancel->RequestCancellation(ECoreKitActionEndReason::Replaced);
		}
		for (const auto& ActionToCancel : ActionsToCancel)
		{
			if (IsValid(ActionToCancel) && GetActiveAction(ActionToCancel->GetActionTag()) == ActionToCancel)
			{
				return ECoreKitActionRequestResult::TransitionInProgress;
			}
		}

		// Admission succeeded; interrupt only earlier playbacks, before the new action can play its montage.
		InterruptIASAMontages();

		// Cleanup may publish a new state or restriction. The original permission snapshot
		// cannot authorize activation after those external callbacks have returned.
		const FGuid RequestId = Report.RequestId;
		Report = EvaluatePolicy(ActionTag, ActivationInputTag, true);
		Report.RequestId = RequestId;
		if (Report.Result != ECoreKitActionRequestResult::Activated)
		{
			return Report.Result;
		}
		bool bCurrentPersistent = false;
		const auto* CurrentEntry = FindActionEntry(ActionTag, bCurrentPersistent);
		if (!CurrentEntry || CurrentEntry->Definition != Definition || bCurrentPersistent != bPersistent ||
			CurrentEntry->ActionTemplate != Entry.ActionTemplate)
		{
			Report.FailureCode = TEXT("ActionConfigurationChangedDuringCleanup");
			return ECoreKitActionRequestResult::InvalidConfiguration;
		}
		if (Context != nullptr && (!IsValid(Context) || !Context->IsActionContextValid()))
		{
			Report.FailureCode = TEXT("InvalidActionContextAfterCleanup");
			Report.Detail = TEXT("The request context became invalid while conflicts were being cancelled.");
			return ECoreKitActionRequestResult::InvalidContext;
		}
		{
			TGuardValue<bool> PolicyGuard(bEvaluatingActionPolicy, true);
			if (!NewAction->EvaluateActivation(Report.FailureCode, Report.Detail))
			{
				return ECoreKitActionRequestResult::RejectedByAction;
			}
		}

		if (bPersistent)
		{
			PersistentActions.Add(ActionTag, NewAction);
		}
		else
		{
			LastAction = CurrentAction;
			CurrentAction = NewAction;
			ActiveActions.Add(ActionTag, NewAction);
		}

		Report.ExecutionId = NewAction->GetExecutionId();
		ObjectClaimSources.Add(NewAction, NewAction);
		AddActiveTagFromSource(ActionTag, NewAction);
		if (!IsActionStillRegistered(NewAction, ActionTag, bPersistent))
		{
			return ECoreKitActionRequestResult::Activated;
		}
		for (const FGameplayTag& GrantedTag : Definition->GrantedTagsWhileActive)
		{
			AddActiveTagFromSource(GrantedTag, NewAction);
			if (!IsActionStillRegistered(NewAction, ActionTag, bPersistent))
			{
				return ECoreKitActionRequestResult::Activated;
			}
		}

		for (const FGameplayTag& TagToClear : Definition->TagsToClearOnStart)
		{
			ClearActiveTag(TagToClear);
			if (!IsActionStillRegistered(NewAction, ActionTag, bPersistent))
			{
				return ECoreKitActionRequestResult::Activated;
			}
		}

		OnActionStarted.Broadcast(ActionTag, NewAction);
		if (!IsActionStillRegistered(NewAction, ActionTag, bPersistent) || NewAction->IsFinished())
		{
			return ECoreKitActionRequestResult::Activated;
		}
		AddTaskReadyForActivation(*NewAction);
	}

	TryActivateQueuedAction();
	return ECoreKitActionRequestResult::Activated;
}

bool UCoreKitActionComponent::CancelAction(const FGameplayTag ActionTag, const bool bForce)
{
	UCoreKitAction* Action = GetActiveAction(ActionTag);
	if (!IsValid(Action) || !CanCancelActionTag(ActionTag, bForce))
	{
		return false;
	}

	return Action->RequestCancellation(ECoreKitActionEndReason::Cancelled, bForce);
}

bool UCoreKitActionComponent::CancelExecution(const FGuid ExecutionId, const bool bForce)
{
	if (!ExecutionId.IsValid())
	{
		return false;
	}

	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : ActiveActions)
	{
		if (IsValid(Pair.Value) && Pair.Value->GetExecutionId() == ExecutionId)
		{
			return CanCancelActionTag(Pair.Key, bForce) &&
				Pair.Value->RequestCancellation(ECoreKitActionEndReason::Cancelled, bForce);
		}
	}
	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : PersistentActions)
	{
		if (IsValid(Pair.Value) && Pair.Value->GetExecutionId() == ExecutionId)
		{
			return CanCancelActionTag(Pair.Key, bForce) &&
				Pair.Value->RequestCancellation(ECoreKitActionEndReason::Cancelled, bForce);
		}
	}
	return false;
}

void UCoreKitActionComponent::CancelAllActions(const bool bIncludePersistentActions, const bool bForce)
{
	TArray<TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>> ActionsToCancel;
	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : ActiveActions)
	{
		ActionsToCancel.Add(Pair);
	}

	if (bIncludePersistentActions)
	{
		for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : PersistentActions)
		{
			ActionsToCancel.Add(Pair);
		}
	}

	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : ActionsToCancel)
	{
		if (IsValid(Pair.Value) && CanCancelActionTag(Pair.Key, bForce))
		{
			Pair.Value->RequestCancellation(ECoreKitActionEndReason::Cancelled, bForce);
		}
	}
}

bool UCoreKitActionComponent::QueueAction(
	const FGameplayTag ActionTag, const FGameplayTag ActivationInputTag, UCoreKitActionContext* Context,
	const FGuid RequestId, const int32 Priority, const float WindowSeconds)
{
	ExpireQueuedActionIfNeeded();
	if (QueuedActionTag.IsValid() && Priority < QueuedPriority)
	{
		return false;
	}

	QueuedActionTag = ActionTag;
	QueuedInputTag = ActivationInputTag;
	QueuedContext = Context;
	QueuedRequestId = RequestId.IsValid() ? RequestId : FGuid::NewGuid();
	QueuedPriority = Priority;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	QueuedExpiresAtSeconds = Now + FMath::Max(0.01f, WindowSeconds);
	ScheduleQueuedExpiry();
	return true;
}

bool UCoreKitActionComponent::ExpireQueuedActionIfNeeded()
{
	if (!QueuedActionTag.IsValid())
	{
		return false;
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Now <= QueuedExpiresAtSeconds)
	{
		return false;
	}
	const FGameplayTag ActionTag = QueuedActionTag;
	const FGameplayTag InputTag = QueuedInputTag;
	const FGuid RequestId = QueuedRequestId;
	ClearQueuedAction();
	RecordQueueTerminal(ActionTag, InputTag, RequestId, ECoreKitActionRequestResult::RequestExpired,
		TEXT("QueueWindowExpired"), TEXT("The queued request exceeded its configured queue window."));
	return true;
}

void UCoreKitActionComponent::ScheduleQueuedExpiry()
{
	if (UWorld* World = GetWorld())
	{
		const double Remaining = FMath::Max(0.0, QueuedExpiresAtSeconds - World->GetTimeSeconds());
		World->GetTimerManager().SetTimer(QueuedExpiryTimer, this, &ThisClass::HandleQueuedExpiry,
			static_cast<float>(Remaining), false);
	}
}

void UCoreKitActionComponent::HandleQueuedExpiry()
{
	ExpireQueuedActionIfNeeded();
}

void UCoreKitActionComponent::RecordQueueTerminal(const FGameplayTag ActionTag, const FGameplayTag InputTag,
	const FGuid RequestId, const ECoreKitActionRequestResult Result, const FName FailureCode, const FString& Detail)
{
	FCoreKitActionRequestReport Report = EvaluatePolicy(ActionTag, InputTag);
	Report.RequestId = RequestId;
	Report.Result = Result;
	Report.Status = ECoreKitActionRequestStatus::Rejected;
	Report.FailureCode = FailureCode;
	Report.Detail = Detail;
	RecordRequest(Report);
}

void UCoreKitActionComponent::TryActivateQueuedAction()
{
	if (!bInitialized || bProcessingQueue || MutationDepth > 0 || !QueuedActionTag.IsValid())
	{
		return;
	}

	TGuardValue<bool> ProcessingGuard(bProcessingQueue, true);
	const FGameplayTag ActionTag = QueuedActionTag;
	const FGameplayTag InputTag = QueuedInputTag;
	UCoreKitActionContext* Context = QueuedContext;
	const FGuid RequestId = QueuedRequestId;
	const int32 Priority = QueuedPriority;
	const double ExpiresAt = QueuedExpiresAtSeconds;
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	ClearQueuedAction();
	ProcessingQueuedRequestId = RequestId;
	bProcessingQueuedRequestCancelled = false;
	ON_SCOPE_EXIT
	{
		ProcessingQueuedRequestId.Invalidate();
		bProcessingQueuedRequestCancelled = false;
	};

	if (Now > ExpiresAt)
	{
		RecordQueueTerminal(ActionTag, InputTag, RequestId, ECoreKitActionRequestResult::RequestExpired,
			TEXT("QueueWindowExpired"), TEXT("The queued request exceeded its configured queue window."));
		return;
	}

	const ECoreKitActionRequestResult Result = RequestActionInternal(
		ActionTag, InputTag, false, false, Context, RequestId).Result;

	if (!bProcessingQueuedRequestCancelled && Result != ECoreKitActionRequestResult::Activated && IsRetryableQueueResult(Result))
	{
		bool bPersistent = false;
		const FCoreKitActionEntry* Entry = FindActionEntry(ActionTag, bPersistent);
		if (!bPersistent && Entry != nullptr && IsValid(Entry->Definition) &&
			Entry->Definition->bQueueWhenBlocked && Now <= ExpiresAt)
		{
			// A delegate may have accepted a newer request while this retry was in flight.
			// Re-arbitrate instead of overwriting it.
			if (!QueuedActionTag.IsValid())
			{
				QueuedActionTag = ActionTag;
				QueuedInputTag = InputTag;
				QueuedContext = Context;
				QueuedRequestId = RequestId;
				QueuedPriority = Priority;
				QueuedExpiresAtSeconds = ExpiresAt;
				ScheduleQueuedExpiry();
			}
		}
	}
}

bool UCoreKitActionComponent::IsRetryableQueueResult(const ECoreKitActionRequestResult Result)
{
	return Result == ECoreKitActionRequestResult::MissingRequirements ||
		Result == ECoreKitActionRequestResult::BlockedByTags ||
		Result == ECoreKitActionRequestResult::RejectedByAction ||
		Result == ECoreKitActionRequestResult::TransitionInProgress;
}

void UCoreKitActionComponent::ClearQueuedAction()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueuedExpiryTimer);
	}
	QueuedActionTag = FGameplayTag();
	QueuedInputTag = FGameplayTag();
	QueuedContext = nullptr;
	QueuedRequestId.Invalidate();
	QueuedPriority = TNumericLimits<int32>::Lowest();
	QueuedExpiresAtSeconds = 0.0;
}

bool UCoreKitActionComponent::CancelQueuedRequest(const FGuid RequestId)
{
	if (RequestId.IsValid() && RequestId == ProcessingQueuedRequestId)
	{
		bProcessingQueuedRequestCancelled = true;
		return true;
	}
	if (!RequestId.IsValid() || RequestId != QueuedRequestId)
	{
		return false;
	}
	ClearQueuedAction();
	return true;
}

UCoreKitAction* UCoreKitActionComponent::GetActiveAction(const FGameplayTag ActionTag) const
{
	if (const TObjectPtr<UCoreKitAction>* Action = ActiveActions.Find(ActionTag))
	{
		return Action->Get();
	}

	if (const TObjectPtr<UCoreKitAction>* Action = PersistentActions.Find(ActionTag))
	{
		return Action->Get();
	}

	return nullptr;
}

TArray<UCoreKitAction*> UCoreKitActionComponent::GetActiveActions() const
{
	TArray<UCoreKitAction*> Result;
	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : ActiveActions)
	{
		if (IsValid(Pair.Value))
		{
			Result.Add(Pair.Value);
		}
	}
	for (const TPair<FGameplayTag, TObjectPtr<UCoreKitAction>>& Pair : PersistentActions)
	{
		if (IsValid(Pair.Value))
		{
			Result.AddUnique(Pair.Value);
		}
	}
	return Result;
}

UCoreKitActionDefinition* UCoreKitActionComponent::GetActionDefinition(const FGameplayTag ActionTag) const
{
	bool bPersistent = false;
	const FCoreKitActionEntry* Entry = FindActionEntry(ActionTag, bPersistent);
	return Entry != nullptr ? Entry->Definition.Get() : nullptr;
}

void UCoreKitActionComponent::RefreshActorContext()
{
	if (!bEndingPlay)
	{
		ActorContext.Refresh(GetOwner());
	}
}

void UCoreKitActionComponent::EnsureActorContext() const
{
	if (!bEndingPlay && !ActorContext.IsPrepared())
	{
		ActorContext.Refresh(GetOwner());
	}
}

UActorComponent* UCoreKitActionComponent::GetCachedComponentByClass(
	const TSubclassOf<UActorComponent> ComponentClass, ECoreKitActorComponentLookupResult& Result,
	const FName ComponentName) const
{
	EnsureActorContext();
	return ActorContext.FindByClass(ComponentClass, Result, ComponentName);
}

UActorComponent* UCoreKitActionComponent::GetCachedComponentByInterface(
	const TSubclassOf<UInterface> InterfaceClass, ECoreKitActorComponentLookupResult& Result,
	const FName ComponentName) const
{
	EnsureActorContext();
	return ActorContext.FindByInterface(InterfaceClass, Result, ComponentName);
}

void UCoreKitActionComponent::HandleActionFinished(
	UCoreKitAction* Action, const ECoreKitActionEndReason Reason)
{
	// EndTask has already marked the task as garbage, but this synchronous callback
	// still owns the object and must remove its exact registration.
	if (Action == nullptr)
	{
		return;
	}

	const FGameplayTag ActionTag = Action->GetActionTag();
	{
		TGuardValue<int32> MutationGuard(MutationDepth, MutationDepth + 1);

		if (ActiveActions.FindRef(ActionTag) == Action)
		{
			ActiveActions.Remove(ActionTag);
		}

		if (PersistentActions.FindRef(ActionTag) == Action)
		{
			PersistentActions.Remove(ActionTag);
		}

		RemoveAllActiveTagsFromSource(Action);
		ObjectClaimSources.Remove(Action);
		ReleaseIASABlocks(Action);
		if (const UCoreKitActionDefinition* Definition = Action->GetActionDefinition())
		{
			if (!Definition->bApplyEndTagsOnlyOnCompleted || Reason == ECoreKitActionEndReason::Completed)
			{
				ApplyEndTagChanges(*Definition);
			}
		}

		if (CurrentAction == Action)
		{
			CurrentAction = nullptr;
		}

		OnActionFinished.Broadcast(ActionTag, Action, Reason);
	}

	TryActivateQueuedAction();
}

bool UCoreKitActionComponent::IsActionStillRegistered(
	const UCoreKitAction* Action, const FGameplayTag ActionTag, const bool bPersistent) const
{
	if (!IsValid(Action) || Action->IsFinished() || bEndingPlay)
	{
		return false;
	}
	return bPersistent ? PersistentActions.FindRef(ActionTag) == Action : ActiveActions.FindRef(ActionTag) == Action;
}

UCoreKitActionSet* UCoreKitActionComponent::ResolveActionSet() const
{
	if (IsValid(ActionSet))
	{
		return ActionSet;
	}

	AActor* Owner = GetOwner();
	if (IsValid(Owner) && Owner->GetClass()->ImplementsInterface(UCoreKitActionSource::StaticClass()))
	{
		return ICoreKitActionSource::Execute_GetAFActionSet(Owner);
	}

	return nullptr;
}

const FCoreKitActionEntry* UCoreKitActionComponent::FindActionEntry(
	const FGameplayTag ActionTag, bool& bOutPersistent) const
{
	bOutPersistent = false;
	if (!IsValid(ActionSet))
	{
		return nullptr;
	}

	if (const FCoreKitActionEntry* Entry = ActionSet->Actions.Find(ActionTag))
	{
		return Entry;
	}

	bOutPersistent = true;
	return ActionSet->PersistentActions.Find(ActionTag);
}

bool UCoreKitActionComponent::CanCancelActionTag(const FGameplayTag ActionTag, const bool bForce) const
{
	const auto* Action = GetActiveAction(ActionTag);
	return bForce || ((!IsValid(ActionSet) || !ActionTag.MatchesAny(ActionSet->NonCancelableActionTags)) &&
		(!IsValid(Action) || Action->CanCancelAction()));
}
