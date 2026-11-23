#include "CoreKitActions/Actions/AsyncNodes/CoreKitWaitForActionTag.h"

#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UCoreKitWaitForActionTag* UCoreKitWaitForActionTag::WaitForActionTag(
	AActor* Actor,
	const FGameplayTag TargetTag,
	const float TimeoutSeconds
)
{
	UCoreKitWaitForActionTag* Node = NewObject<UCoreKitWaitForActionTag>();
	Node->ObservedActor = Actor;
	Node->TargetTag = TargetTag;
	Node->TimeoutSeconds = TimeoutSeconds;
	return Node;
}

void UCoreKitWaitForActionTag::Activate()
{
	if (bActivated || bCompleted)
	{
		return;
	}
	bActivated = true;
	if (!IsValid(ObservedActor) || !IsValid(ObservedActor->GetWorld()) || !TargetTag.IsValid())
	{
		Finish(ECoreKitWaitForActionTagResult::InvalidSource);
		return;
	}

	ActionComponent = ObservedActor->FindComponentByClass<UCoreKitActionComponent>();
	if (!ActionComponent.IsValid())
	{
		Finish(ECoreKitWaitForActionTagResult::InvalidSource);
		return;
	}

	RegisterWithGameInstance(ObservedActor);
	if (ActionComponent->HasActiveTag(TargetTag))
	{
		Finish(ECoreKitWaitForActionTagResult::Completed);
		return;
	}

	ActionComponent->OnActiveTagAdded.AddDynamic(this, &ThisClass::HandleTagAdded);
	ActionComponent->OnActionComponentEnding.AddDynamic(this, &ThisClass::HandleActionComponentEnding);
	ObservedActor->OnEndPlay.AddDynamic(this, &ThisClass::HandleOwnerEndPlay);
	if (TimeoutSeconds > 0.0f)
	{
		ObservedActor->GetWorldTimerManager().SetTimer(
			TimeoutHandle, this, &ThisClass::HandleTimeout, TimeoutSeconds, false);
	}
	Super::Activate();
}

void UCoreKitWaitForActionTag::HandleTagAdded(const FGameplayTag AddedTag)
{
	if (!bCompleted && AddedTag.MatchesTag(TargetTag))
	{
		Finish(ECoreKitWaitForActionTagResult::Completed);
	}
}

void UCoreKitWaitForActionTag::HandleOwnerEndPlay(AActor* Actor, const EEndPlayReason::Type EndPlayReason)
{
	Finish(ECoreKitWaitForActionTagResult::OwnerEnded);
}

void UCoreKitWaitForActionTag::HandleActionComponentEnding()
{
	Finish(ECoreKitWaitForActionTagResult::ComponentEnded);
}

void UCoreKitWaitForActionTag::HandleTimeout()
{
	Finish(ECoreKitWaitForActionTagResult::TimedOut);
}

void UCoreKitWaitForActionTag::Cancel()
{
	if (bCompleted)
	{
		return;
	}

	Finish(ECoreKitWaitForActionTagResult::Cancelled);
}

void UCoreKitWaitForActionTag::Finish(const ECoreKitWaitForActionTagResult FinishResult)
{
	if (bCompleted)
	{
		return;
	}

	bCompleted = true;
	Result = FinishResult;
	Unbind();
	if (Result == ECoreKitWaitForActionTagResult::Completed)
	{
		OnCompleted.Broadcast();
	}
	OnFinished.Broadcast(Result);
	SetReadyToDestroy();
}

void UCoreKitWaitForActionTag::Unbind()
{
	if (UCoreKitActionComponent* Component = ActionComponent.Get())
	{
		Component->OnActiveTagAdded.RemoveDynamic(this, &ThisClass::HandleTagAdded);
		Component->OnActionComponentEnding.RemoveDynamic(this, &ThisClass::HandleActionComponentEnding);
	}
	if (IsValid(ObservedActor))
	{
		ObservedActor->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleOwnerEndPlay);
		if (UWorld* World = ObservedActor->GetWorld())
		{
			World->GetTimerManager().ClearTimer(TimeoutHandle);
		}
	}
}

void UCoreKitWaitForActionTag::BeginDestroy()
{
	Unbind();
	Super::BeginDestroy();
}
