#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"

bool UCoreKitCommandAction::EvaluateActivation_Implementation(FName& Code, FString& Detail) const
{
	Code = TEXT("CommandRequiresTypedDispatch");
	Detail = TEXT(
		"Use ExecuteActionCommand with a typed domain adapter. RequestAction cannot supply command payloads.");
	return false;
}

bool UCoreKitCommandAction::ValidatePolicy(const UCoreKitActionDefinition* D, FString& Detail)
{
	if (!IsValid(D) || D->bQueueWhenBlocked || D->bTickAction || D->bCancelOtherActions || D->bClosesIASAOnStart
		|| !D->ActionsToCancel.IsEmpty() || !D->GrantedTagsWhileActive.IsEmpty()
		|| !D->TagsToClearOnStart.IsEmpty() || !D->TagsToAddOnEnd.IsEmpty() || !D->TagsToClearOnEnd.IsEmpty())
	{
		Detail = TEXT(
			"Command policies cannot queue, tick, cancel tasks or mutate lifecycle tags. Use a direct UCoreKitAction for sessions.");
		return false;
	}
	return true;
}
