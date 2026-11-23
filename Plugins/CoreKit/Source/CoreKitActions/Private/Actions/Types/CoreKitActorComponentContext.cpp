#include "CoreKitActions/Actions/Types/CoreKitActorComponentContext.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

void FCoreKitActorComponentContext::Refresh(AActor* InOwner)
{
	Owner = InOwner;
	Components.Reset();
	bPrepared = true;
	++Revision;

	if (!IsValid(InOwner))
	{
		return;
	}

	TInlineComponentArray<UActorComponent*> OwnerComponents(InOwner);
	for (UActorComponent* Component : OwnerComponents)
	{
		if (IsValid(Component))
		{
			Components.Add(Component);
		}
	}
}

void FCoreKitActorComponentContext::Reset()
{
	Owner.Reset();
	Components.Reset();
	bPrepared = false;
	++Revision;
}

UActorComponent* FCoreKitActorComponentContext::FindByClass(const TSubclassOf<UActorComponent> ComponentClass,
	ECoreKitActorComponentLookupResult& OutResult, const FName ComponentName) const
{
	if (!IsValid(Owner.Get()))
	{
		OutResult = ECoreKitActorComponentLookupResult::OwnerUnavailable;
		return nullptr;
	}
	if (!*ComponentClass)
	{
		OutResult = ECoreKitActorComponentLookupResult::InvalidType;
		return nullptr;
	}

	UActorComponent* Match = nullptr;
	for (const TWeakObjectPtr<UActorComponent>& Candidate : Components)
	{
		UActorComponent* Component = Candidate.Get();
		if (!IsValid(Component) || !Component->IsA(ComponentClass) ||
			(!ComponentName.IsNone() && Component->GetFName() != ComponentName))
		{
			continue;
		}
		if (Match != nullptr)
		{
			OutResult = ECoreKitActorComponentLookupResult::Ambiguous;
			return nullptr;
		}
		Match = Component;
	}

	OutResult = Match ? ECoreKitActorComponentLookupResult::Found : ECoreKitActorComponentLookupResult::NotFound;
	return Match;
}

UActorComponent* FCoreKitActorComponentContext::FindByInterface(const TSubclassOf<UInterface> InterfaceClass,
	ECoreKitActorComponentLookupResult& OutResult, const FName ComponentName) const
{
	if (!IsValid(Owner.Get()))
	{
		OutResult = ECoreKitActorComponentLookupResult::OwnerUnavailable;
		return nullptr;
	}
	if (!*InterfaceClass)
	{
		OutResult = ECoreKitActorComponentLookupResult::InvalidType;
		return nullptr;
	}

	UActorComponent* Match = nullptr;
	for (const TWeakObjectPtr<UActorComponent>& Candidate : Components)
	{
		UActorComponent* Component = Candidate.Get();
		if (!IsValid(Component) || !Component->GetClass()->ImplementsInterface(InterfaceClass) ||
			(!ComponentName.IsNone() && Component->GetFName() != ComponentName))
		{
			continue;
		}
		if (Match != nullptr)
		{
			OutResult = ECoreKitActorComponentLookupResult::Ambiguous;
			return nullptr;
		}
		Match = Component;
	}

	OutResult = Match ? ECoreKitActorComponentLookupResult::Found : ECoreKitActorComponentLookupResult::NotFound;
	return Match;
}
