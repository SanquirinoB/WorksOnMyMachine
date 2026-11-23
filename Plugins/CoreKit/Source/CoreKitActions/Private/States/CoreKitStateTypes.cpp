#include "CoreKitActions/States/CoreKitStateTypes.h"

bool FCoreKitStateDomainDefinition::Validate(FString& Reason, const bool bProvider) const
{
	if (Domain.IsNone() || States.IsEmpty() || !States.HasTagExact(InitialState))
	{
		Reason = TEXT("A domain, explicit allowed states and an allowed initial state are required.");
		return false;
	}
	for (const auto& State : States)
	{
		if (!State.IsValid() || !State.ToString().StartsWith(TEXT("State.")))
		{
			Reason = TEXT("State domains may only reserve valid State.* tags.");
			return false;
		}
	}
	TSet<FString> SeenEdges;
	for (const auto& Edge : Transitions)
	{
		if (!States.HasTagExact(Edge.From) || !States.HasTagExact(Edge.To)
			|| !Edge.Event.IsValid() || !Edge.Event.ToString().StartsWith(TEXT("Event.")))
		{
			Reason = TEXT("Every edge requires allowed From/To states and a registered Event.* tag.");
			return false;
		}
		const FString Key = Edge.From.ToString() + TEXT("|") + Edge.Event.ToString();
		if (SeenEdges.Contains(Key))
		{
			Reason = TEXT("An event must have one unambiguous destination from a given state.");
			return false;
		}
		SeenEdges.Add(Key);
	}
	if (!bProvider && States.Num() > 1 && Transitions.IsEmpty())
	{
		Reason = TEXT("A managed domain with multiple states requires explicit transitions.");
		return false;
	}
	Reason.Reset();
	return true;
}
