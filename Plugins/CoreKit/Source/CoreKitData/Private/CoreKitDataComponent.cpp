#include "CoreKitData/CoreKitDataComponent.h"

#include "CoreKitData/CoreKitDataContainer.h"

UCoreKitDataContainer* UCoreKitDataComponent::FindDataContainer(FGameplayTag Tag) const
{
	if (!Tag.IsValid())
	{
		return nullptr;
	}

	const TObjectPtr<UCoreKitDataContainer>* Container = Containers.Find(Tag);
	return Container && IsValid(*Container) ? Container->Get() : nullptr;
}
