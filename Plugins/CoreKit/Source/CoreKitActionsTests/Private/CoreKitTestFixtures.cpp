#include "CoreKitTestFixtures.h"

#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"

bool UCoreKitTestAction::EvaluateActivation_Implementation(FName& FailureCode, FString& Detail) const
{
	if (!bAllowActivation)
	{
		FailureCode = TEXT("TestActionNotReady");
		Detail = TEXT("The test fixture rejected activation.");
		return false;
	}

	return Super::EvaluateActivation_Implementation(FailureCode, Detail);
}

void UCoreKitTestAction::ReceiveCancelRequested_Implementation(const ECoreKitActionEndReason Reason)
{
	if (!bDelayCancellation)
	{
		FinishCancellation();
	}
}

void UCoreKitTestAction::Activate()
{
	Super::Activate();
	if (ACoreKitTestOwner* Owner = Cast<ACoreKitTestOwner>(GetActionOwner()))
	{
		++Owner->Starts;
		Owner->CopiedValue = AuthoredValue;
	}
}

void UCoreKitTestAction::CleanupAction(const ECoreKitActionEndReason Reason)
{
	if (ACoreKitTestOwner* Owner = Cast<ACoreKitTestOwner>(GetActionOwner()))
	{
		++Owner->Cleanups;
		Owner->LastCleanupReason = Reason;
	}
	Super::CleanupAction(Reason);
}

void UCoreKitTestTagListener::HandleTagAdded(const FGameplayTag AddedTag)
{
	if (AddedTag == FinishWhenAdded && Actions)
	{
		if (UCoreKitAction* Action = Actions->GetActiveAction(ActionToFinish))
		{
			Action->CompleteAction();
		}
	}
}

void UCoreKitTestTickAction::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (ACoreKitTestOwner* Owner = Cast<ACoreKitTestOwner>(GetActionOwner()))
	{
		++Owner->Ticks;
	}
}

bool UCoreKitTestAsyncAction::CompleteFromCallback(const FGuid CallbackExecutionId)
{
	if (!CallbackExecutionId.IsValid() || CallbackExecutionId != GetExecutionId() || IsFinished())
	{
		return false;
	}

	if (IsEnding())
	{
		FinishCancellation();
	}
	else
	{
		CompleteAction();
	}
	return true;
}
