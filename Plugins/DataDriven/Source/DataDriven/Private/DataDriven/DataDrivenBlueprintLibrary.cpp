#include "DataDriven/DataDrivenBlueprintLibrary.h"

#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/DataFragmentBase.h"

UDataFragmentBase* UDataDrivenBlueprintLibrary::GetDataFragmentByClass(
	UDataContainer* InDataContainer,
	const TSubclassOf<UDataFragmentBase> FragmentClass,
	EDataDrivenLookupResult& Result)
{
	Result = EDataDrivenLookupResult::Success;

	if (!IsValid(InDataContainer))
	{
		Result = EDataDrivenLookupResult::InvalidContainer;
		return nullptr;
	}

	const UClass* TargetClass = FragmentClass.Get();
	if (!IsValid(TargetClass))
	{
		Result = EDataDrivenLookupResult::InvalidFragmentClass;
		return nullptr;
	}

	for (const TObjectPtr<UDataFragmentBase>& DataFragment : InDataContainer->GetAllDataFragments())
	{
		if (IsValid(DataFragment) && DataFragment->IsA(TargetClass))
		{
			return DataFragment.Get();
		}
	}

	Result = EDataDrivenLookupResult::FragmentNotFound;
	return nullptr;
}

TArray<UDataFragmentBase*> UDataDrivenBlueprintLibrary::GetDataFragmentsByClass(
	UDataContainer* InDataContainer,
	const TSubclassOf<UDataFragmentBase> FragmentClass,
	EDataDrivenLookupResult& Result)
{
	TArray<UDataFragmentBase*> FoundFragments;
	Result = EDataDrivenLookupResult::Success;

	if (!IsValid(InDataContainer))
	{
		Result = EDataDrivenLookupResult::InvalidContainer;
		return FoundFragments;
	}

	const UClass* TargetClass = FragmentClass.Get();
	if (!IsValid(TargetClass))
	{
		Result = EDataDrivenLookupResult::InvalidFragmentClass;
		return FoundFragments;
	}

	for (const TObjectPtr<UDataFragmentBase>& DataFragment : InDataContainer->GetAllDataFragments())
	{
		if (IsValid(DataFragment) && DataFragment->IsA(TargetClass))
		{
			FoundFragments.Add(DataFragment.Get());
		}
	}

	if (FoundFragments.IsEmpty())
	{
		Result = EDataDrivenLookupResult::FragmentNotFound;
	}

	return FoundFragments;
}

bool UDataDrivenBlueprintLibrary::HasDataFragmentOfClass(
	UDataContainer* InDataContainer,
	const TSubclassOf<UDataFragmentBase> FragmentClass)
{
	EDataDrivenLookupResult Result = EDataDrivenLookupResult::FragmentNotFound;
	GetDataFragmentByClass(InDataContainer, FragmentClass, Result);
	return Result == EDataDrivenLookupResult::Success;
}

UDataContainer* UDataDrivenBlueprintLibrary::GetDataContainerByTag(
	UDataDrivenComponent* InComponent,
	const FGameplayTag ContainerTag,
	EDataDrivenLookupResult& Result)
{
	Result = EDataDrivenLookupResult::Success;

	if (!IsValid(InComponent))
	{
		Result = EDataDrivenLookupResult::InvalidContainer;
		return nullptr;
	}

	if (!ContainerTag.IsValid())
	{
		Result = EDataDrivenLookupResult::InvalidTag;
		return nullptr;
	}

	if (UDataContainer* Container = InComponent->GetDataContainer(ContainerTag))
	{
		return Container;
	}

	Result = EDataDrivenLookupResult::ContainerNotFound;
	return nullptr;
}
