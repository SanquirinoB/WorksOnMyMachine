#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"

#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/Types/CoreKitInterruptibility.h"

#if WITH_EDITOR

EDataValidationResult UCoreKitActionDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (GrantedTagsWhileActive.HasTagExact(CoreKitActionTags::IASA) || TagsToAddOnEnd.HasTagExact(CoreKitActionTags::IASA))
	{
		Context.AddError(FText::FromString(TEXT("State.Action.IASA is derived by the action coordinator. Use the IASA policy and scoped readiness API.")));
		Result = EDataValidationResult::Invalid;
	}

	if (RequiredTags.HasAny(BlockedTags))
	{
		Context.AddError(FText::FromString(TEXT("RequiredTags and BlockedTags overlap.")));
		Result = EDataValidationResult::Invalid;
	}

	if (TagsToAddOnEnd.HasAny(TagsToClearOnEnd))
	{
		Context.AddError(FText::FromString(TEXT("TagsToAddOnEnd and TagsToClearOnEnd overlap.")));
		Result = EDataValidationResult::Invalid;
	}

	if (!TagsToClearOnStart.IsEmpty() || !TagsToClearOnEnd.IsEmpty())
	{
		Context.AddError(FText::FromString(
			TEXT("Global tag clearing is not supported. Use owner-scoped claims or an authoritative State transition.")));
		Result = EDataValidationResult::Invalid;
	}

	if (bQueueWhenBlocked && QueueWindowSeconds <= 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("QueueWindowSeconds must be greater than zero when queueing is enabled.")));
		Result = EDataValidationResult::Invalid;
	}

	if (ActionsToCancel.HasAny(GrantedTagsWhileActive))
	{
		Context.AddWarning(FText::FromString(
			TEXT("ActionsToCancel overlaps GrantedTagsWhileActive; verify action tags are not being used as state grants.")));
	}

	return Result;
}

EDataValidationResult UCoreKitActionSet::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	auto ValidateMap = [&Context, &Result](
		const TMap<FGameplayTag, FCoreKitActionEntry>& ActionMap,
		const TCHAR* MapName
	)
	{
		for (const TPair<FGameplayTag, FCoreKitActionEntry>& Pair : ActionMap)
		{
			if (IsValid(Pair.Value.ActionTemplate) && Pair.Value.ActionTemplate->IsA<UCoreKitCommandAction>())
			{
				FString Detail;
				if (FCString::Strcmp(MapName, TEXT("PersistentActions")) == 0
					|| !UCoreKitCommandAction::ValidatePolicy(Pair.Value.Definition, Detail))
				{
					Context.AddError(FText::FromString(
						FString::Printf(TEXT("Invalid command '%s': %s"), *Pair.Key.ToString(), *Detail)));
					Result = EDataValidationResult::Invalid;
				}
			}
			if (!Pair.Key.IsValid())
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("%s contains an invalid action tag key."), MapName)
				));
				Result = EDataValidationResult::Invalid;
			}

			if (!IsValid(Pair.Value.ActionTemplate))
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("%s entry '%s' has no Action Template."), MapName, *Pair.Key.ToString())
				));
				Result = EDataValidationResult::Invalid;
			}

			if (!IsValid(Pair.Value.Definition))
			{
				Context.AddError(FText::FromString(
					FString::Printf(TEXT("%s entry '%s' has no Definition."), MapName, *Pair.Key.ToString())
				));
				Result = EDataValidationResult::Invalid;
			}
		}
	};

	ValidateMap(Actions, TEXT("Actions"));
	ValidateMap(PersistentActions, TEXT("PersistentActions"));

	for (const TPair<FGameplayTag, FCoreKitActionEntry>& Pair : Actions)
	{
		if (PersistentActions.Contains(Pair.Key))
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Action tag '%s' exists in both Actions and PersistentActions."),
				*Pair.Key.ToString()
			)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

#endif
