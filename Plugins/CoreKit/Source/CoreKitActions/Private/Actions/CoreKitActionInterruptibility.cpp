#include "CoreKitActions/Actions/CoreKitAction.h"

#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

void UCoreKitAction::SetIASA(const bool bReady)
{
	UCoreKitActionComponent* Actions = GetActionComponent();
	if (!IsValid(Actions) || IsEnding())
	{
		return;
	}
	if (bReady)
	{
		const bool bReleased = Actions->ReleaseIASA(IASABlockHandle);
		IASABlockHandle.Invalidate();
		if (bReleased && !IsEnding())
		{
			OnIASAReleased();
		}
	}
	else if (!IASABlockHandle.IsValid())
	{
		IASABlockHandle = Actions->BlockIASA(this, GetActionTag().GetTagName());
	}
}

bool UCoreKitAction::BindIASAMontage(UAnimInstance* Animation, const int32 MontageInstanceId)
{
	UCoreKitActionComponent* Actions = GetActionComponent();
	return IsValid(Actions) && Actions->BindIASAToMontage(IASABlockHandle, Animation, MontageInstanceId);
}

bool UCoreKitAction::BindIASAAnimation(UAnimInstance* Animation, UAnimMontage* Montage)
{
	const FAnimMontageInstance* Instance = IsValid(Animation) && IsValid(Montage)
		? Animation->GetActiveInstanceForMontage(Montage) : nullptr;
	return Instance && BindIASAMontage(Animation, Instance->GetInstanceID());
}
