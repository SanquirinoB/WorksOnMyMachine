#include "CoreKitExampleHeldIntent.h"

#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/CoreKitAction.h"

void UCoreKitExampleHeldIntent::Configure(UCoreKitActionComponent* InActions, FGameplayTag InActionTag)
{
	ClearBinding();
	Actions = InActions;
	ActionTag = InActionTag;
	if (InActions)
	{
		InActions->OnIASAChanged.AddUniqueDynamic(this, &ThisClass::ReadinessChanged);
		InActions->OnActionFinished.AddUniqueDynamic(this, &ThisClass::ActionFinished);
		InActions->OnActionComponentEnding.AddUniqueDynamic(this, &ThisClass::CoordinatorEnding);
	}
}

void UCoreKitExampleHeldIntent::SetHeld(bool bInHeld)
{
	if (bHeld == bInHeld)
	{
		return;
	}

	bHeld = bInHeld;
	if (bHeld)
	{
		TryStart();
	}
	else if (auto* Coordinator = Actions.Get(); Coordinator && Execution.IsValid())
	{
		const FGuid Released = Execution;
		Execution.Invalidate();
		Coordinator->CancelExecution(Released);
	}
}

void UCoreKitExampleHeldIntent::ReadinessChanged(bool bReady)
{
	if (bReady)
	{
		TryStart();
	}
}

void UCoreKitExampleHeldIntent::TryStart()
{
	auto* Coordinator = Actions.Get();
	if (!bHeld || bRequesting || Execution.IsValid() || !Coordinator || !Coordinator->IsIASA())
	{
		return;
	}

	TGuardValue<bool> Guard(bRequesting, true);
	const auto Report = Coordinator->RequestActionWithContext(ActionTag, FGameplayTag(), nullptr);
	if (Report.Status == ECoreKitActionRequestStatus::Started)
	{
		if (const auto* Active = Coordinator->GetActiveAction(ActionTag); Active && Active->GetExecutionId() == Report.ExecutionId)
		{
			if (bHeld)
			{
				Execution = Report.ExecutionId;
			}
			else
			{
				Coordinator->CancelExecution(Report.ExecutionId);
			}
		}
	}
	else if (Report.Status == ECoreKitActionRequestStatus::Queued)
	{
		Coordinator->CancelQueuedRequest(Report.RequestId);
	}
}

void UCoreKitExampleHeldIntent::ActionFinished(FGameplayTag, UCoreKitAction* Action, ECoreKitActionEndReason)
{
	if (Action && Action->GetExecutionId() == Execution)
	{
		Execution.Invalidate();
	}
}

void UCoreKitExampleHeldIntent::CoordinatorEnding()
{
	bHeld = false;
	Execution.Invalidate();
	ClearBinding();
}

void UCoreKitExampleHeldIntent::ClearBinding()
{
	SetHeld(false);
	Execution.Invalidate();
	if (auto* Coordinator = Actions.Get())
	{
		Coordinator->OnIASAChanged.RemoveDynamic(this, &ThisClass::ReadinessChanged);
		Coordinator->OnActionFinished.RemoveDynamic(this, &ThisClass::ActionFinished);
		Coordinator->OnActionComponentEnding.RemoveDynamic(this, &ThisClass::CoordinatorEnding);
	}
	Actions.Reset();
}

void UCoreKitExampleHeldIntent::EndPlay(EEndPlayReason::Type Reason)
{
	ClearBinding();
	Super::EndPlay(Reason);
}
