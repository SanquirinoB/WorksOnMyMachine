#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CoreKitActions/Actions/CoreKitAction.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace CoreKitActionTags
{
	UE_DEFINE_GAMEPLAY_TAG(IASA, "State.Action.IASA");
}

FGuid UCoreKitActionComponent::BlockIASA(UObject* Source, const FName Reason)
{
	if (!IsValid(Source) || bEndingPlay || bEvaluatingActionPolicy)
	{
		return FGuid();
	}

	const FGuid Handle = FGuid::NewGuid();
	FCoreKitIASABlock& Block = IASABlocks.Add(Handle);
	Block.Source = Source;
	Block.Reason = Reason;
	RefreshIASA();
	return Handle;
}

bool UCoreKitActionComponent::ReleaseIASA(const FGuid Handle)
{
	const FCoreKitIASABlock* Block = IASABlocks.Find(Handle);
	if (bEvaluatingActionPolicy || !Block)
	{
		return false;
	}
	RememberInterruptibleMontage(Block->Animation.Get(), Block->MontageInstanceId);
	IASABlocks.Remove(Handle);
	RefreshIASA();
	return true;
}

bool UCoreKitActionComponent::HasIASABlock(const FGuid Handle) const
{
	const FCoreKitIASABlock* Block = IASABlocks.Find(Handle);
	return Block && Block->Source.IsValid();
}

void UCoreKitActionComponent::ReleaseIASABlocks(UObject* Source)
{
	if (!Source || bEvaluatingActionPolicy)
	{
		return;
	}

	for (auto Iterator = IASABlocks.CreateIterator(); Iterator; ++Iterator)
	{
		if (Iterator.Value().Source.Get() == Source)
		{
			Iterator.RemoveCurrent();
		}
	}
	RefreshIASA();
}

void UCoreKitActionComponent::SetIASAStateBlock(UObject* Source, const FName Reason, const bool bBlocked)
{
	if (!IsValid(Source) || bEndingPlay || bEvaluatingActionPolicy)
	{
		return;
	}

	for (auto Iterator = IASABlocks.CreateIterator(); Iterator; ++Iterator)
	{
		const FCoreKitIASABlock& Block = Iterator.Value();
		if (Block.bStateProjection && Block.Source == Source && Block.Reason == Reason)
		{
			if (!bBlocked)
			{
				Iterator.RemoveCurrent();
				RefreshIASA();
			}
			return;
		}
	}

	if (bBlocked)
	{
		FCoreKitIASABlock& Block = IASABlocks.Add(FGuid::NewGuid());
		Block.Source = Source;
		Block.Reason = Reason;
		Block.bStateProjection = true;
		RefreshIASA();
	}
}

bool UCoreKitActionComponent::BindIASAToMontage(const FGuid Handle, UAnimInstance* Animation,
	const int32 MontageInstanceId)
{
	FCoreKitIASABlock* Block = IASABlocks.Find(Handle);
	if (!Block || !IsValid(Animation) || Animation->GetOwningActor() != GetOwner()
		|| !Animation->GetMontageInstanceForID(MontageInstanceId))
	{
		return false;
	}
	Block->Animation = Animation;
	Block->MontageInstanceId = MontageInstanceId;
	InterruptibleMontages.RemoveAll([&](const FCoreKitIASAMontage& Playback)
	{
		return Playback.Animation == Animation && Playback.MontageInstanceId == MontageInstanceId;
	});
	Animation->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &ThisClass::HandleIASAMontageNotify);
	IASAAnimations.Add(Animation);
	return true;
}

void UCoreKitActionComponent::HandleIASAMontageNotify(const FName NotifyName,
	const FBranchingPointNotifyPayload& Payload)
{
	if (NotifyName == TEXT("IASA") && Payload.SkelMeshComponent)
	{
		ReleaseIASAForMontage(Payload.SkelMeshComponent->GetAnimInstance(), Payload.MontageInstanceID);
	}
}

void UCoreKitActionComponent::UnbindIASAAnimations()
{
	for (const TWeakObjectPtr<UAnimInstance>& Animation : IASAAnimations)
	{
		if (Animation.IsValid())
		{
			Animation->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ThisClass::HandleIASAMontageNotify);
		}
	}
	IASAAnimations.Reset();
	InterruptibleMontages.Reset();
}

void UCoreKitActionComponent::ObserveIASAAnimations()
{
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(GetOwner());
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (UAnimInstance* Animation = Mesh->GetAnimInstance())
		{
			Animation->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &ThisClass::HandleIASAMontageNotify);
			IASAAnimations.Add(Animation);
		}
	}
}

bool UCoreKitActionComponent::RememberInterruptibleMontage(UAnimInstance* Animation, const int32 MontageInstanceId)
{
	if (bEndingPlay || !IsValid(Animation) || Animation->GetOwningActor() != GetOwner()
		|| !Animation->GetMontageInstanceForID(MontageInstanceId))
	{
		return false;
	}
	InterruptibleMontages.RemoveAll([](const FCoreKitIASAMontage& Playback)
	{
		UAnimInstance* Instance = Playback.Animation.Get();
		return !Instance || !Instance->GetMontageInstanceForID(Playback.MontageInstanceId);
	});
	if (!InterruptibleMontages.ContainsByPredicate([&](const FCoreKitIASAMontage& Playback)
	{
		return Playback.Animation == Animation && Playback.MontageInstanceId == MontageInstanceId;
	}))
	{
		InterruptibleMontages.Add({Animation, MontageInstanceId});
	}
	return true;
}

int32 UCoreKitActionComponent::InterruptIASAMontages(const float BlendOutSeconds)
{
	if (!IsIASA() || bEvaluatingActionPolicy || bInterruptingIASAMontages)
	{
		return 0;
	}
	TGuardValue<bool> Guard(bInterruptingIASAMontages, true);
	const TArray<FCoreKitIASAMontage> Pending = InterruptibleMontages;
	int32 Interrupted = 0;
	for (const FCoreKitIASAMontage& Playback : Pending)
	{
		if (!IsIASA())
		{
			break;
		}
		InterruptibleMontages.RemoveAll([&](const FCoreKitIASAMontage& Current)
		{
			return Current.Animation == Playback.Animation && Current.MontageInstanceId == Playback.MontageInstanceId;
		});
		UAnimInstance* Animation = Playback.Animation.Get();
		FAnimMontageInstance* Instance = Animation && Animation->GetOwningActor() == GetOwner()
			? Animation->GetMontageInstanceForID(Playback.MontageInstanceId) : nullptr;
		if (Instance)
		{
			// Stop the timeline immediately: no later commit notify may fire during the visual blend.
			Instance->SetPlaying(false);
			Instance->Stop(FAlphaBlend(FMath::Max(0.0f, BlendOutSeconds)));
			++Interrupted;
		}
	}
	return Interrupted;
}

bool UCoreKitActionComponent::ReleaseIASAForMontage(UAnimInstance* Animation, const int32 MontageInstanceId)
{
	if (bEvaluatingActionPolicy || !RememberInterruptibleMontage(Animation, MontageInstanceId))
	{
		return false;
	}

	TArray<FGuid> Handles;
	for (const auto& Pair : IASABlocks)
	{
		const FCoreKitIASABlock& Block = Pair.Value;
		if (!Block.bStateProjection && Block.Animation == Animation && Block.MontageInstanceId == MontageInstanceId)
		{
			Handles.Add(Pair.Key);
		}
	}
	for (const FGuid Handle : Handles)
	{
		const FCoreKitIASABlock* Block = IASABlocks.Find(Handle);
		if (UCoreKitAction* Action = Block ? Cast<UCoreKitAction>(Block->Source.Get()) : nullptr)
		{
			Action->SetIASA(true);
		}
		else
		{
			ReleaseIASA(Handle);
		}
	}
	return true;
}

TArray<FName> UCoreKitActionComponent::GetIASABlockReasons() const
{
	TArray<FName> Reasons;
	for (const auto& Pair : IASABlocks)
	{
		if (Pair.Value.Source.IsValid())
		{
			Reasons.AddUnique(Pair.Value.Reason);
		}
	}
	Reasons.Sort(FNameLexicalLess());
	return Reasons;
}

#if WITH_EDITOR
TArray<FString> UCoreKitActionComponent::DescribeIASABlocks() const
{
	TArray<FString> Lines;
	for (const auto& Entry : IASABlocks)
	{
		if (const UObject* Source = Entry.Value.Source.Get())
		{
			Lines.Add(FString::Printf(TEXT("%s: %s"), *Entry.Value.Reason.ToString(), *Source->GetPathName()));
		}
	}
	Lines.Sort();
	return Lines;
}
#endif

void UCoreKitActionComponent::RefreshIASA()
{
	for (auto Iterator = IASABlocks.CreateIterator(); Iterator; ++Iterator)
	{
		if (!Iterator.Value().Source.IsValid())
		{
			Iterator.RemoveCurrent();
		}
	}

	const bool bReady = !bEndingPlay && IASABlocks.IsEmpty();
	const bool bChanged = bIASA != bReady;
	bIASA = bReady;
	const FGameplayTagContainer PreviousTags = ActiveTags;
	RefreshActiveTags();
	{
		// Publish a consistent gate before observers can request another action.
		TGuardValue<int32> Guard(MutationDepth, MutationDepth + 1);
		BroadcastTagChanges(PreviousTags);
	}
	if (bChanged && bIASA == bReady)
	{
		OnIASAChanged.Broadcast(bIASA);
	}
	if (bChanged && bIASA && !bEndingPlay)
	{
		TryActivateQueuedAction();
	}
}
