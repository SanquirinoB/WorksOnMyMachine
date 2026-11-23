#include "CoreKitActions/Actions/CoreKitAction.h"

#include "CoreKitActions/Actions/CoreKitActionContext.h"
#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActionsLog.h"
#include "GameFramework/Actor.h"

UCoreKitAction::UCoreKitAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCaresAboutPriority = true;
}

UCoreKitAction* UCoreKitAction::CreateAction(
	IGameplayTaskOwnerInterface& TaskOwner,
	UObject* Outer,
	UClass* ActionClass,
	UCoreKitAction* ActionTemplate
)
{
	if (!IsValid(Outer) || !IsValid(ActionClass) || !ActionClass->IsChildOf(StaticClass()))
	{
		return nullptr;
	}

	UCoreKitAction* NewAction = NewObject<UCoreKitAction>(Outer, ActionClass, NAME_None, RF_Transient,
	                                                                    ActionTemplate);
	if (IsValid(NewAction))
	{
		NewAction->InitTask(TaskOwner, TaskOwner.GetGameplayTaskDefaultPriority());
	}

	return NewAction;
}

void UCoreKitAction::InitializeAction(
	UCoreKitActionComponent* InComponent,
	const FGameplayTag InActionTag,
	const FGameplayTag InActivationInputTag,
	UCoreKitActionDefinition* InDefinition,
	UCoreKitActionContext* InContext
)
{
	ActionComponent = InComponent;
	OwnerActor = InComponent ? InComponent->GetOwner() : nullptr;
	ActionTag = InActionTag;
	ActivationInputTag = InActivationInputTag;
	Definition = InDefinition;
	Context = InContext;
	ExecutionId = FGuid::NewGuid();
	bTickingTask = InDefinition != nullptr && InDefinition->bTickAction;
}

bool UCoreKitAction::CanActivateAction_Implementation() const
{
	return OwnerActor.IsValid() && ActionComponent.IsValid() &&
		(Context == nullptr || (IsValid(Context) && Context->IsActionContextValid()));
}

void UCoreKitAction::CompleteAction()
{
	FinishAction(ECoreKitActionEndReason::Completed);
}

void UCoreKitAction::CancelAction()
{
	RequestCancellation(ECoreKitActionEndReason::Cancelled);
}

void UCoreKitAction::FailAction()
{
	FinishAction(ECoreKitActionEndReason::Failed);
}

bool UCoreKitAction::EvaluateActivation_Implementation(FName& FailureCode, FString& Detail) const
{
	const bool bAllowed = CanActivateAction();
	FailureCode = bAllowed ? NAME_None : FName(TEXT("RejectedByAction"));
	Detail = bAllowed ? FString() : TEXT("CanActivateAction rejected the request.");
	return bAllowed;
}

bool UCoreKitAction::CanCancelAction_Implementation() const { return true; }

bool UCoreKitAction::RequestCancellation(const ECoreKitActionEndReason Reason, const bool bForce)
{
	if (bFinishRequested) { return false; }
	if (bForce)
	{
		FinishAction(Reason);
		return true;
	}
	if (bCancelRequested) { return true; }
	if (!CanCancelAction()) { return false; }
	bCancelRequested = true;
	PendingEndReason = Reason;
	SetActionPhase(TEXT("Closing"));
	ReceiveCancelRequested(Reason);
	return true;
}

void UCoreKitAction::ReceiveCancelRequested_Implementation(const ECoreKitActionEndReason Reason)
{
	FinishCancellation();
}

void UCoreKitAction::FinishCancellation()
{
	if (bCancelRequested) { FinishAction(PendingEndReason); }
}

void UCoreKitAction::SetActionPhase(const FName Phase)
{
	if (bFinalized || ActionPhase == Phase) { return; }
	ActionPhase = Phase;
	if (const auto* Component = GetActionComponent(); Component && Component->bLogActionRequests)
	{
		UE_LOG(LogCoreKitActions, Display, TEXT("Action %s [%s] phase=%s owner=%s"),
		       *ActionTag.ToString(), *ExecutionId.ToString(), *Phase.ToString(), *GetNameSafe(GetActionOwner()));
	}
}

void UCoreKitAction::FinishAction(const ECoreKitActionEndReason Reason)
{
	if (bFinishRequested)
	{
		return;
	}

	bFinishRequested = true;
	PendingEndReason = Reason;
	UCoreKitActionComponent* Component = ActionComponent.Get();
	int32 UnownedMutationDepth = 0;
	{
		// Hold the coordinator through Blueprint cleanup AND derived OnDestroy cleanup.
		int32& Depth = Component ? Component->MutationDepth : UnownedMutationDepth;
		TGuardValue<int32> Guard(Depth, Depth + 1);
		ReceiveEndAction(Reason);
		EndTask();
	}
	if (Component) { Component->TryActivateQueuedAction(); }
}

void UCoreKitAction::Activate()
{
	Super::Activate();
	if (Definition.IsValid() && Definition->bClosesIASAOnStart)
	{
		SetIASA(false);
	}
	if (bFinishRequested || IsFinished())
	{
		return;
	}
	ElapsedTime = 0.0f;
	SetActionPhase(TEXT("Active"));
	OnActionActivated.Broadcast();
	if (bFinishRequested || IsFinished())
	{
		return;
	}
	ReceiveActivateAction();
}

void UCoreKitAction::ExternalCancel()
{
	RequestCancellation(ECoreKitActionEndReason::ExternalCancelled);
}

void UCoreKitAction::OnDestroy(const bool bInOwnerFinished)
{
	if (bFinalized) { return; }
	UCoreKitActionComponent* Component = ActionComponent.Get();
	int32 UnownedMutationDepth = 0;
	{
		int32& Depth = Component ? Component->MutationDepth : UnownedMutationDepth;
		TGuardValue<int32> Guard(Depth, Depth + 1);
		if (!bFinishRequested)
		{
			bFinishRequested = true;
			PendingEndReason = bInOwnerFinished
				                   ? ECoreKitActionEndReason::OwnerEndPlay
				                   : ECoreKitActionEndReason::ExternalCancelled;
			ReceiveEndAction(PendingEndReason);
		}
		SetActionPhase(TEXT("Finished"));
		bFinalized = true;
		CleanupAction(PendingEndReason);
		// Engine marks the task Finished here; completed observers must never see Active.
		Super::OnDestroy(bInOwnerFinished);
		if (Component) { Component->HandleActionFinished(this, PendingEndReason); }
		OnActionFinishedNative.Broadcast(this, PendingEndReason);
		OnActionEnded.Broadcast(PendingEndReason);
		ElapsedTime = 0.0f;
	}
	if (Component) { Component->TryActivateQueuedAction(); }
}

void UCoreKitAction::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);
	ElapsedTime += DeltaTime;
	ReceiveTickAction(DeltaTime);
}

void UCoreKitAction::AddOwnedTag(const FGameplayTag Tag) const
{
	if (!bFinishRequested && !IsFinished())
	{
		if (UCoreKitActionComponent* Component = ActionComponent.Get())
		{
			Component->AddActiveTagFromSource(Tag, this);
		}
	}
}

void UCoreKitAction::RemoveOwnedTag(const FGameplayTag Tag) const
{
	if (UCoreKitActionComponent* Component = ActionComponent.Get())
	{
		Component->RemoveActiveTagFromSource(Tag, this);
	}
}

bool UCoreKitAction::HasActiveTag(const FGameplayTag Tag) const
{
	const UCoreKitActionComponent* Component = ActionComponent.Get();
	return Component != nullptr && Component->HasActiveTag(Tag);
}

bool UCoreKitAction::HasAllActiveTags(const FGameplayTagContainer& Tags) const
{
	const UCoreKitActionComponent* Component = ActionComponent.Get();
	return Component != nullptr && Component->HasAllActiveTags(Tags);
}

UActorComponent* UCoreKitAction::GetCachedOwnerComponentByClass(
	const TSubclassOf<UActorComponent> ComponentClass, ECoreKitActorComponentLookupResult& Result,
	const FName ComponentName) const
{
	const UCoreKitActionComponent* Component = ActionComponent.Get();
	if (Component == nullptr)
	{
		Result = ECoreKitActorComponentLookupResult::OwnerUnavailable;
		return nullptr;
	}
	return Component->GetCachedComponentByClass(ComponentClass, Result, ComponentName);
}
