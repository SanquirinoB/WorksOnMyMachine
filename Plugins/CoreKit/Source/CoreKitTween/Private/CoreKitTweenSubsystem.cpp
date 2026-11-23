#include "CoreKitTween/CoreKitTweenSubsystem.h"

#include "Engine/World.h"

namespace
{
float ApplyEase(float Alpha, ECoreKitTweenEase Ease)
{
	switch (Ease)
	{
	case ECoreKitTweenEase::SmoothStep:
		return Alpha * Alpha * (3.0f - 2.0f * Alpha);
	case ECoreKitTweenEase::QuadraticIn:
		return FMath::Square(Alpha);
	case ECoreKitTweenEase::QuadraticOut:
		return 1.0f - FMath::Square(1.0f - Alpha);
	case ECoreKitTweenEase::QuadraticInOut:
		return FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	default:
		return Alpha;
	}
}
}

uint64 UCoreKitTweenSubsystem::StartTween(UObject* Owner, float Duration, ECoreKitTweenEase Ease,
	FCoreKitTweenUpdated OnUpdate, FCoreKitTweenFinished OnFinish)
{
	FTween Tween;
	Tween.Owner = Owner;
	Tween.Steps.Add({ Duration, Ease });
	Tween.OnUpdate = MoveTemp(OnUpdate);
	Tween.OnFinish = MoveTemp(OnFinish);
	return AddTween(MoveTemp(Tween));
}

uint64 UCoreKitTweenSubsystem::StartSequence(UObject* Owner, const TArray<FCoreKitTweenStep>& Steps,
	const FCoreKitTweenOptions& Options, FCoreKitTweenStepUpdated OnUpdate, FCoreKitTweenFinished OnFinish)
{
	FTween Tween;
	Tween.Owner = Owner;
	Tween.Steps = Steps;
	Tween.Options = Options;
	Tween.OnStepUpdated = MoveTemp(OnUpdate);
	Tween.OnFinish = MoveTemp(OnFinish);
	return AddTween(MoveTemp(Tween));
}

uint64 UCoreKitTweenSubsystem::AddTween(FTween&& Tween)
{
	if (!IsInitialized() || !Tween.Owner.IsValid() || Tween.Steps.IsEmpty()
		|| (!Tween.OnUpdate.IsBound() && !Tween.OnStepUpdated.IsBound())
		|| Tween.Steps.ContainsByPredicate([](const auto& Step) { return !FMath::IsFinite(Step.Duration) || Step.Duration < 0.0f; }))
	{
		return 0;
	}

	if (!Tween.Options.Channel.IsNone() && Tween.Options.Conflict != ECoreKitTweenConflict::Allow)
	{
		const auto Conflicts = FindChannelTweens(Tween.Owner.Get(), Tween.Options.Channel);
		if (!Conflicts.IsEmpty() && Tween.Options.Conflict == ECoreKitTweenConflict::Reject)
		{
			return 0;
		}
		for (uint64 Conflict : Conflicts)
		{
			FinishTween(Conflict, false);
		}
		// A callback can replace the old tween with its own request or destroy the owner.
		if (!IsInitialized() || !Tween.Owner.IsValid() || !FindChannelTweens(Tween.Owner.Get(), Tween.Options.Channel).IsEmpty())
		{
			return 0;
		}
	}

	const uint64 Handle = ++LastHandle;
	Tween.LastRealTime = FPlatformTime::Seconds();
	Tweens.Add(Handle, MoveTemp(Tween));
	return Handle;
}

TArray<uint64> UCoreKitTweenSubsystem::FindChannelTweens(UObject* Owner, FName Channel) const
{
	TArray<uint64> Handles;
	for (const auto& Entry : Tweens)
	{
		if (Entry.Value.Owner.Get() == Owner && Entry.Value.Options.Channel == Channel)
		{
			Handles.Add(Entry.Key);
		}
	}
	return Handles;
}

int32 UCoreKitTweenSubsystem::CancelTweensByChannel(UObject* Owner, FName Channel)
{
	int32 Count = 0;
	if (IsValid(Owner))
	{
		for (uint64 Handle : FindChannelTweens(Owner, Channel))
		{
			Count += FinishTween(Handle, false) ? 1 : 0;
		}
	}
	return Count;
}

bool UCoreKitTweenSubsystem::SetTweenPaused(uint64 Handle, bool bPaused)
{
	if (FTween* Tween = Tweens.Find(Handle))
	{
		if (Tween->bPaused != bPaused)
		{
			Tween->LastRealTime = FPlatformTime::Seconds();
		}
		Tween->bPaused = bPaused;
		return true;
	}
	return false;
}

bool UCoreKitTweenSubsystem::CancelTween(uint64 Handle)
{
	return FinishTween(Handle, false);
}

bool UCoreKitTweenSubsystem::FinishTween(uint64 Handle, bool bCompleted)
{
	FTween Removed;
	if (!Tweens.RemoveAndCopyValue(Handle, Removed))
	{
		return false;
	}

	Removed.OnFinish.ExecuteIfBound(bCompleted);
	return true;
}

void UCoreKitTweenSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Callbacks may remove an entry or add a tween for the next tick.
	TArray<uint64> ActiveHandles;
	Tweens.GenerateKeyArray(ActiveHandles);
	const double Now = FPlatformTime::Seconds();
	const bool bWorldPaused = GetWorld()->IsPaused();
	for (uint64 Handle : ActiveHandles)
	{
		FTween* Tween = Tweens.Find(Handle);
		if (!Tween)
		{
			continue;
		}
		if (!Tween->Owner.IsValid() || (!Tween->OnUpdate.IsBound() && !Tween->OnStepUpdated.IsBound()))
		{
			FinishTween(Handle, false);
			continue;
		}
		const bool bRealTime = Tween->Options.Clock == ECoreKitTweenClock::RealTime;
		const double Budget = bRealTime ? Now - Tween->LastRealTime : static_cast<double>(DeltaTime);
		Tween->LastRealTime = Now;
		if (!Tween->bPaused && (bRealTime || !bWorldPaused))
		{
			AdvanceTween(Handle, FMath::Max(0.0, Budget));
		}
	}
}

void UCoreKitTweenSubsystem::AdvanceTween(uint64 Handle, double Budget)
{
	while (FTween* Tween = Tweens.Find(Handle))
	{
		if (Tween->bPaused)
		{
			return;
		}
		if (!Tween->Owner.IsValid())
		{
			FinishTween(Handle, false);
			return;
		}
		const int32 Index = Tween->StepIndex;
		const FCoreKitTweenStep Step = Tween->Steps[Index];
		const double Remaining = Step.Duration - Tween->Elapsed;
		const bool bStepCompleted = Budget >= Remaining;
		const double Used = FMath::Min(Budget, Remaining);
		Tween->Elapsed += Used;
		Budget -= Used;
		const float Alpha = ApplyEase(bStepCompleted ? 1.0f : static_cast<float>(Tween->Elapsed / Step.Duration), Step.Ease);
		if (Tween->OnStepUpdated.IsBound())
		{
			const auto Callback = Tween->OnStepUpdated;
			Callback.ExecuteIfBound(Index, Alpha);
		}
		else
		{
			const auto Callback = Tween->OnUpdate;
			Callback.ExecuteIfBound(Alpha);
		}
		Tween = Tweens.Find(Handle);
		if (!Tween || !bStepCompleted)
		{
			return;
		}
		if (++Tween->StepIndex == Tween->Steps.Num())
		{
			FinishTween(Handle, Tween->Owner.IsValid());
			return;
		}
		Tween->Elapsed = 0.0;
		if (Budget <= 0.0 && Tween->Steps[Tween->StepIndex].Duration > 0.0f)
		{
			return;
		}
	}
}

void UCoreKitTweenSubsystem::Deinitialize()
{
	Super::Deinitialize();
	TMap<uint64, FTween> Cancelled = MoveTemp(Tweens);
	for (auto& Entry : Cancelled)
	{
		Entry.Value.OnFinish.ExecuteIfBound(false);
	}
}

bool UCoreKitTweenSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

TStatId UCoreKitTweenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCoreKitTweenSubsystem, STATGROUP_Tickables);
}
