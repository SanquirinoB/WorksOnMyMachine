#include "DataDriven/DataContainer.h"

#if WITH_EDITOR
EDataValidationResult UDataContainer::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	bool bHasInvalidFragment = false;

	for (int32 Index = 0; Index < DataFragments.Num(); ++Index)
	{
		if (!IsValid(DataFragments[Index]))
		{
			Context.AddError(FText::Format(
				FText::FromString(TEXT("DataDriven container contains an invalid fragment at index {0}.")),
				FText::AsNumber(Index)));
			bHasInvalidFragment = true;
		}
	}

	if (bHasInvalidFragment || ParentResult == EDataValidationResult::Invalid)
	{
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}
#endif

const TArray<TObjectPtr<UDataFragmentBase>>& UDataContainer::GetAllDataFragments() const
{
	return DataFragments;
}

TArray<UDataFragmentBase*> UDataContainer::BP_GetAllDataFragments() const
{
	TArray<UDataFragmentBase*> Result;
	Result.Reserve(DataFragments.Num());

	for (const TObjectPtr<UDataFragmentBase>& DataFragment : DataFragments)
	{
		if (IsValid(DataFragment))
		{
			Result.Add(DataFragment.Get());
		}
	}

	return Result;
}
