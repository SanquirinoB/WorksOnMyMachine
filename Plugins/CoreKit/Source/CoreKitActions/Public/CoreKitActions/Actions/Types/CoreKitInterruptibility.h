#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

class UAnimInstance;

namespace CoreKitActionTags
{
	COREKITACTIONS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(IASA);
}

/** One exact reason that an actor cannot yet perform voluntary gameplay. Never persisted. */
struct FCoreKitIASABlock
{
	TWeakObjectPtr<UObject> Source;
	FName Reason;
	TWeakObjectPtr<UAnimInstance> Animation;
	int32 MontageInstanceId = INDEX_NONE;
	bool bStateProjection = false;
};

/** A playback that reached IASA; its identity remains independent of asset replays and other meshes. */
struct FCoreKitIASAMontage
{
	TWeakObjectPtr<UAnimInstance> Animation;
	int32 MontageInstanceId = INDEX_NONE;
};
