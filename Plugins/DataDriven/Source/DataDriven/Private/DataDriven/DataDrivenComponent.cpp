#include "DataDriven/DataDrivenComponent.h"

UDataDrivenComponent::UDataDrivenComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UDataContainer* UDataDrivenComponent::GetDataContainer(const FGameplayTag InTag) const
{
	if (!InTag.IsValid())
	{
		return nullptr;
	}

	if (const TObjectPtr<UDataContainer>* FoundContainer = DataContainerMap.Find(InTag))
	{
		return FoundContainer->Get();
	}

	return nullptr;
}

bool UDataDrivenComponent::HasDataContainer(const FGameplayTag InTag) const
{
	return GetDataContainer(InTag) != nullptr;
}

void UDataDrivenComponent::SetDataContainerMap(const TMap<FGameplayTag, TObjectPtr<UDataContainer>>& InDataContainerMap)
{
	DataContainerMap = InDataContainerMap;
}

void UDataDrivenComponent::SetDataContainers(const TMap<FGameplayTag, UDataContainer*>& InDataContainers)
{
	DataContainerMap.Reset();
	DataContainerMap.Reserve(InDataContainers.Num());
	for (const TPair<FGameplayTag, UDataContainer*>& Pair : InDataContainers)
	{
		SetDataContainer(Pair.Key, Pair.Value);
	}
}

bool UDataDrivenComponent::SetDataContainer(const FGameplayTag InTag, UDataContainer* InDataContainer)
{
	if (!InTag.IsValid() || !IsValid(InDataContainer))
	{
		return false;
	}

	DataContainerMap.Add(InTag, InDataContainer);
	return true;
}

bool UDataDrivenComponent::RemoveDataContainer(const FGameplayTag InTag)
{
	return InTag.IsValid() && DataContainerMap.Remove(InTag) > 0;
}

void UDataDrivenComponent::ClearDataContainerMap()
{
	DataContainerMap.Reset();
}

TMap<FGameplayTag, UDataContainer*> UDataDrivenComponent::BP_GetDataContainers() const
{
	TMap<FGameplayTag, UDataContainer*> Result;
	Result.Reserve(DataContainerMap.Num());

	for (const TPair<FGameplayTag, TObjectPtr<UDataContainer>>& Pair : DataContainerMap)
	{
		if (Pair.Key.IsValid() && IsValid(Pair.Value))
		{
			Result.Add(Pair.Key, Pair.Value.Get());
		}
	}

	return Result;
}
