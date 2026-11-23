#include "CoreKitActions/Actions/Animation/CoreKitAnimNotify_IASA.h"

#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UCoreKitAnimNotify_IASA::UCoreKitAnimNotify_IASA()
{
	bIsNativeBranchingPoint = true;
}

void UCoreKitAnimNotify_IASA::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
	USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
	AActor* Owner = IsValid(Mesh) ? Mesh->GetOwner() : nullptr;
	UCoreKitActionComponent* Actions = IsValid(Owner) ? Owner->FindComponentByClass<UCoreKitActionComponent>() : nullptr;
	if (IsValid(Actions))
	{
		Actions->ReleaseIASAForMontage(Mesh->GetAnimInstance(), Payload.MontageInstanceID);
	}
}

FString UCoreKitAnimNotify_IASA::GetNotifyName_Implementation() const
{
	return TEXT("IASA");
}
