#include "CoreKitTween/CoreKitTweenAction.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

UCoreKitTweenAction* UCoreKitTweenAction::PlayTween(const UObject* WorldContextObject, UObject* Owner,
	float Duration, ECoreKitTweenEase Ease)
{
	return PlayTweenSequence(WorldContextObject, Owner, { { Duration, Ease } }, {});
}

UCoreKitTweenAction* UCoreKitTweenAction::PlayTweenSequence(const UObject* WorldContextObject, UObject* Owner,
	const TArray<FCoreKitTweenStep>& Steps, const FCoreKitTweenOptions& Options)
{
	UCoreKitTweenAction* Action = NewObject<UCoreKitTweenAction>();
	Action->TweenOwner = Owner;
	Action->StepsToPlay = Steps;
	Action->TweenOptions = Options;

	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		Action->Subsystem = World->GetSubsystem<UCoreKitTweenSubsystem>();
		Action->RegisterWithGameInstance(World->GetGameInstance());
	}
	return Action;
}

void UCoreKitTweenAction::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	if (UCoreKitTweenSubsystem* Runner = Subsystem.Get())
	{
		Handle = Runner->StartSequence(TweenOwner.Get(), StepsToPlay, TweenOptions,
			FCoreKitTweenStepUpdated::CreateUObject(this, &ThisClass::Update),
			FCoreKitTweenFinished::CreateUObject(this, &ThisClass::Finish));
	}
	if (Handle == 0)
	{
		Finish(false);
	}
}

void UCoreKitTweenAction::SetPaused(bool bPaused)
{
	if (UCoreKitTweenSubsystem* Runner = Subsystem.Get())
	{
		Runner->SetTweenPaused(Handle, bPaused);
	}
}

void UCoreKitTweenAction::Cancel()
{
	if (UCoreKitTweenSubsystem* Runner = Subsystem.Get())
	{
		Runner->CancelTween(Handle);
	}
}

void UCoreKitTweenAction::Update(int32 StepIndex, float Alpha)
{
	CurrentStep = StepIndex;
	LastAlpha = Alpha;
	Updated.Broadcast(Alpha);
}

void UCoreKitTweenAction::Finish(bool bCompleted)
{
	Handle = 0;
	if (bCompleted)
	{
		Completed.Broadcast(LastAlpha);
	}
	else
	{
		Cancelled.Broadcast(LastAlpha);
	}
	SetReadyToDestroy();
}
