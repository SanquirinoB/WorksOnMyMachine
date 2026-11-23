#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EndTraversal.generated.h"

UCLASS()
class GASP_API UAnimNotify_EndTraversal : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_EndTraversal();

	virtual FString GetNotifyName_Implementation() const override;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;
};
