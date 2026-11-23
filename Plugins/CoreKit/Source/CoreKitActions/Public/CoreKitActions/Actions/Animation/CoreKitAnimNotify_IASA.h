#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreKitAnimNotify_IASA.generated.h"

/** Reopens global voluntary gameplay for this playback, unless another action/state still blocks it. */
UCLASS(meta = (DisplayName = "IASA"))
class COREKITACTIONS_API UCoreKitAnimNotify_IASA : public UAnimNotify
{
	GENERATED_BODY()

public:
	UCoreKitAnimNotify_IASA();
	virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
	virtual FString GetNotifyName_Implementation() const override;
};
