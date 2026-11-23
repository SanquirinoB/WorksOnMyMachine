#include "Animation/Notifies/AnimNotify_EndTraversal.h"

#include "Components/GASP_TraversalComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_EndTraversal)

UAnimNotify_EndTraversal::UAnimNotify_EndTraversal()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

FString UAnimNotify_EndTraversal::GetNotifyName_Implementation() const
{
	return TEXT("End Traversal");
}

void UAnimNotify_EndTraversal::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                      const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UGASP_TraversalComponent* TraversalComponent = OwnerActor->FindComponentByClass<UGASP_TraversalComponent>();
	if (!IsValid(TraversalComponent))
	{
		return;
	}

	TraversalComponent->CompleteTraversalFromNotify();
}
