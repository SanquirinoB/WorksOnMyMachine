#include "CoreKitActions/States/CoreKitStateComponent.h"
#include "CoreKitActions/States/CoreKitStateProvider.h"
#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAFStates, Log, All);

UCoreKitStateComponent::UCoreKitStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

double UCoreKitStateComponent::Now() const { return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0; }

FCoreKitStateReport UCoreKitStateComponent::MakeReport(
	const ECoreKitStateResult Result, const FName Domain, const FString& Detail) const
{
	FCoreKitStateReport Report;
	Report.Result = Result;
	Report.Domain = Domain;
	Report.Detail = Detail;
	Report.SnapshotRevision = Revision;
	Report.TimeSeconds = Now();
	if (const auto* Entry = Domains.Find(Domain))
	{
		Report.From = Entry->Snapshot.State;
		Report.AuthorityPath = Entry->Snapshot.AuthorityPath;
		Report.ExecutionId = Entry->Snapshot.ExecutionId;
	}
	return Report;
}

void UCoreKitStateComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()) { BindActions(GetOwner()->FindComponentByClass<UCoreKitActionComponent>()); }
}

void UCoreKitStateComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	// Keep the reservation tombstones until destruction. Unexpected removal must fail closed,
	// not silently turn an authoritative pawn back into a legacy pawn with fewer blocks.
	OnStatesSettled.Clear();
	Super::EndPlay(Reason);
}

bool UCoreKitStateComponent::BindActions(UCoreKitActionComponent* Actions)
{
	if (!IsValid(Actions) || Actions->GetOwner() != GetOwner() || IsCommittingTransition() || bEndingPlay)
	{
		return false;
	}
	if (BoundActions.IsValid()) { return BoundActions.Get() == Actions; }
	FGameplayTagContainer Reservations;
	for (const auto& Pair : Domains) { Reservations.AppendTags(Pair.Value.Definition.States); }
	FString Reason;
	if (!Actions->CanReserveStateTags(Reservations, Reason) || !Actions->AttachStateComponent(this)) { return false; }
	BoundActions = Actions;
	return true;
}

FCoreKitStateReport UCoreKitStateComponent::RegisterDomain(
	const FCoreKitStateDomainDefinition& Definition, UObject* Writer, FCoreKitStateAuthority& Authority)
{
	return RegisterInternal(Definition, Writer, Authority, false);
}

FCoreKitStateReport UCoreKitStateComponent::RegisterProvidedDomain(
	const FCoreKitStateDomainDefinition& Definition, UObject* Provider, FCoreKitStateAuthority& Authority)
{
	return RegisterInternal(Definition, Provider, Authority, true);
}

FCoreKitStateReport UCoreKitStateComponent::RegisterInternal(
	const FCoreKitStateDomainDefinition& Definition, UObject* Writer,
	FCoreKitStateAuthority& Authority, const bool bProvider)
{
	Authority = {};
	auto Fail = [&](ECoreKitStateResult Result, const FString& Detail)
	{
		auto Report = MakeReport(Result, Definition.Domain, Detail);
		Publish({Report}, false);
		return Report;
	};
	if (bEndingPlay)
	{
		return Fail(ECoreKitStateResult::OwnerEndingPlay, TEXT("State runtime is ending play."));
	}
	if (IsCommittingTransition())
	{
		return Fail(ECoreKitStateResult::TransitionInProgress,
		            TEXT("Wait until the current state notification completes."));
	}
	if (!IsInGameThread() || !IsValid(Writer) || (GetWorld() && GetWorld()->GetNetMode() == NM_Client))
	{
		return Fail(ECoreKitStateResult::InvalidAuthority,
		            TEXT("A live local writer on the game thread is required; network-client writes are unsupported."));
	}
	FString Reason;
	if (!Definition.Validate(Reason, bProvider)) { return Fail(ECoreKitStateResult::InvalidDefinition, Reason); }
	if (Domains.Contains(Definition.Domain))
	{
		return Fail(ECoreKitStateResult::DuplicateDomain, TEXT("The domain already has an owner."));
	}
	for (const auto& Tag : Definition.States)
	{
		if (IsTagReserved(Tag))
		{
			return Fail(ECoreKitStateResult::OverlappingDomain, TEXT("State hierarchy is already reserved."));
		}
	}
	if (!BoundActions.IsValid() && GetOwner())
	{
		if (auto* Actions = GetOwner()->FindComponentByClass<UCoreKitActionComponent>())
		{
			if (!BindActions(Actions))
			{
				return Fail(ECoreKitStateResult::LegacyClaimConflict,
				            TEXT("Cannot attach the actor's action coordinator."));
			}
		}
	}
	if (BoundActions.IsValid() && !BoundActions->CanReserveStateTags(Definition.States, Reason))
	{
		return Fail(ECoreKitStateResult::LegacyClaimConflict, Reason);
	}
	FGameplayTag Initial = Definition.InitialState;
	if (bProvider)
	{
		if (!Writer->Implements<UCoreKitStateProvider>())
		{
			return Fail(ECoreKitStateResult::InvalidAuthority,
			            TEXT("Provided domains require the state provider interface."));
		}
		// A provider is authoritative already; its enum must not be initialized by this registry.
		TGuardValue<int32> Guard(ProviderQueryDepth, ProviderQueryDepth + 1);
		Initial = ICoreKitStateProvider::Execute_QueryDomainState(Writer, Definition.Domain);
		if (!Definition.States.HasTagExact(Initial))
		{
			return Fail(ECoreKitStateResult::ProviderUnavailable,
			            TEXT("Provider returned an unregistered domain state."));
		}
	}
	FDomain Entry;
	Entry.Definition = Definition;
	Entry.Writer = Writer;
	Entry.Generation = FGuid::NewGuid();
	Entry.Snapshot.Domain = Definition.Domain;
	Entry.Snapshot.State = Initial;
	Entry.Snapshot.Revision = 1;
	Entry.Snapshot.AuthorityPath = Writer->GetPathName();
	Entry.Snapshot.EnteredAtSeconds = Now();
	Entry.Snapshot.bProvider = bProvider;
	Entry.Snapshot.bAuthorityAvailable = true;
	Authority.Domain = Definition.Domain;
	Authority.Generation = Entry.Generation;
	Domains.Add(Definition.Domain, MoveTemp(Entry));
	++Revision;
	auto Report = MakeReport(ECoreKitStateResult::Success, Definition.Domain,
	                         TEXT("Domain registered with exclusive authority."));
	Report.From = {};
	Report.To = Initial;
	Publish({Report}, true);
	return Report;
}

ECoreKitStateResult UCoreKitStateComponent::CheckAuthority(
	const FCoreKitStateAuthority& Authority, UObject* Writer) const
{
	if (bEndingPlay) { return ECoreKitStateResult::OwnerEndingPlay; }
	if (IsCommittingTransition()) { return ECoreKitStateResult::TransitionInProgress; }
	if (!IsInGameThread() || !IsValid(Writer) || (GetWorld() && GetWorld()->GetNetMode() == NM_Client))
	{
		return ECoreKitStateResult::InvalidAuthority;
	}
	const auto* Entry = Domains.Find(Authority.Domain);
	if (!Entry) { return ECoreKitStateResult::UnknownDomain; }
	if (!Authority.Generation.IsValid() || Authority.Generation != Entry->Generation)
	{
		return ECoreKitStateResult::StaleAuthority;
	}
	if (Entry->Writer.Get() != Writer) { return ECoreKitStateResult::InvalidAuthority; }
	return ECoreKitStateResult::Success;
}

FCoreKitStateReport UCoreKitStateComponent::UnregisterDomain(
	const FCoreKitStateAuthority& Authority, UObject* Writer)
{
	auto Report = MakeReport(CheckAuthority(Authority, Writer), Authority.Domain);
	if (Report.Result == ECoreKitStateResult::Success)
	{
		Domains.Remove(Authority.Domain);
		++Revision;
		Report.SnapshotRevision = Revision;
		Report.Detail = TEXT("Owner explicitly released its domain.");
	}
	Publish({Report}, Report.Result == ECoreKitStateResult::Success);
	return Report;
}

FCoreKitStateReport UCoreKitStateComponent::EvaluateTransition(
	const FCoreKitStateTransitionRequest& Request) const
{
	auto Report = MakeReport(CheckAuthority(Request.Authority, Request.Writer), Request.Authority.Domain);
	Report.Event = Request.Event;
	Report.To = Request.To;
	Report.ExecutionId = Request.ExecutionId;
	if (Report.Result != ECoreKitStateResult::Success) { return Report; }
	const auto& Entry = Domains.FindChecked(Request.Authority.Domain);
	if (Entry.Snapshot.bProvider)
	{
		Report.Result = ECoreKitStateResult::ReadOnlyProvider;
		return Report;
	}
	if (Request.ExpectedRevision != Entry.Snapshot.Revision)
	{
		Report.Result = ECoreKitStateResult::StaleRevision;
		return Report;
	}
	if (Request.ExpectedState != Entry.Snapshot.State)
	{
		Report.Result = ECoreKitStateResult::UnexpectedState;
		return Report;
	}
	if (Request.ExpectedExecutionId != Entry.Snapshot.ExecutionId
		|| (Entry.Snapshot.ExecutionId.IsValid() && Request.ExecutionId.IsValid() && Request.ExecutionId != Entry.
			Snapshot.ExecutionId))
	{
		Report.Result = ECoreKitStateResult::StaleExecution;
		return Report;
	}
	const auto* Edge = Entry.Definition.Transitions.FindByPredicate([&](const auto& Rule)
	{
		return Rule.From == Request.ExpectedState && Rule.Event == Request.Event && Rule.To == Request.To;
	});
	if (!Edge)
	{
		Report.Result = ECoreKitStateResult::InvalidTransition;
		return Report;
	}
	FString Health;
	if (!IsHealthy(Health))
	{
		Report.Result = ECoreKitStateResult::ProviderUnavailable;
		Report.Detail = Health;
		return Report;
	}
	FGameplayTagContainer Tags;
	GetOwnedGameplayTags(Tags);
	if (BoundActions.IsValid()) { Tags.AppendTags(BoundActions->GetActiveTags()); }
	for (const auto& Required : Edge->RequiredTags)
	{
		if (!Tags.HasTag(Required)) { Report.MissingTags.AddTag(Required); }
	}
	for (const auto& Active : Tags)
	{
		if (Active.MatchesAny(Edge->BlockedTags)) { Report.BlockingTags.AddTag(Active); }
	}
	if (!Report.MissingTags.IsEmpty()) { Report.Result = ECoreKitStateResult::MissingRequirements; }
	else if (!Report.BlockingTags.IsEmpty()) { Report.Result = ECoreKitStateResult::Blocked; }
	return Report;
}

FCoreKitStateReport UCoreKitStateComponent::TryTransition(
	const FCoreKitStateTransitionRequest& Request)
{
	return TryTransitionBatch({Request});
}

FCoreKitStateReport UCoreKitStateComponent::TryTransitionBatch(
	const TArray<FCoreKitStateTransitionRequest>& Requests)
{
	if (Requests.IsEmpty())
	{
		auto Report = MakeReport(ECoreKitStateResult::InvalidTransition, NAME_None,
		                         TEXT("An empty transition batch is not an operation."));
		Publish({Report}, false);
		return Report;
	}
	TSet<FName> Seen;
	TArray<FCoreKitStateReport> Reports;
	for (const auto& Request : Requests)
	{
		auto Report = EvaluateTransition(Request);
		if (Seen.Contains(Request.Authority.Domain))
		{
			Report.Result = ECoreKitStateResult::InvalidTransition;
			Report.Detail = TEXT("A batch may transition each domain only once.");
		}
		if (Report.Result != ECoreKitStateResult::Success)
		{
			Publish({Report}, false);
			return Report;
		}
		Seen.Add(Request.Authority.Domain);
		Reports.Add(Report);
	}
	// No external callbacks between validation and this commit. No partial batch is observable.
	for (const auto& Request : Requests)
	{
		auto& Snapshot = Domains.FindChecked(Request.Authority.Domain).Snapshot;
		Snapshot.State = Request.To;
		Snapshot.ExecutionId = Request.ExecutionId;
		++Snapshot.Revision;
		Snapshot.EnteredAtSeconds = Now();
	}
	++Revision;
	for (auto& Report : Reports) { Report.SnapshotRevision = Revision; }
	Publish(Reports, true);
	return Reports.Last();
}

FCoreKitStateReport UCoreKitStateComponent::RefreshProvidedDomain(
	const FCoreKitStateAuthority& Authority, UObject* Provider)
{
	auto Report = MakeReport(CheckAuthority(Authority, Provider), Authority.Domain);
	if (Report.Result != ECoreKitStateResult::Success)
	{
		Publish({Report}, false);
		return Report;
	}
	auto& Entry = Domains.FindChecked(Authority.Domain);
	if (!Entry.Snapshot.bProvider)
	{
		Report.Result = ECoreKitStateResult::InvalidAuthority;
		Report.Detail = TEXT("Managed domains require a validated transition, not provider refresh.");
		Publish({Report}, false);
		return Report;
	}
	FGameplayTag State;
	{
		TGuardValue<int32> Guard(ProviderQueryDepth, ProviderQueryDepth + 1);
		State = ICoreKitStateProvider::Execute_QueryDomainState(Provider, Authority.Domain);
	}
	if (!Entry.Definition.States.HasTagExact(State))
	{
		Report.Result = ECoreKitStateResult::ProviderUnavailable;
		Report.Detail = TEXT("Provider has no valid state; the last reservation is retained, not cleared.");
		Publish({Report}, false);
		return Report;
	}
	const bool bChanged = State != Entry.Snapshot.State;
	if (bChanged)
	{
		Entry.Snapshot.State = State;
		++Entry.Snapshot.Revision;
		Entry.Snapshot.EnteredAtSeconds = Now();
		++Revision;
		Report.SnapshotRevision = Revision;
	}
	Report.To = State;
	Publish({Report}, bChanged);
	return Report;
}

FCoreKitStateDomainSnapshot UCoreKitStateComponent::GetDomainState(const FName Domain) const
{
	if (const auto* Entry = Domains.Find(Domain))
	{
		auto Snapshot = Entry->Snapshot;
		Snapshot.bAuthorityAvailable = !bEndingPlay && Entry->Writer.IsValid();
		return Snapshot;
	}
	return {};
}

FCoreKitStateSnapshot UCoreKitStateComponent::GetStateSnapshot() const
{
	FCoreKitStateSnapshot Snapshot;
	Snapshot.Revision = Revision;
	for (const auto& Pair : Domains)
	{
		Snapshot.Domains.Add(GetDomainState(Pair.Key));
		Snapshot.Tags.AddTag(Pair.Value.Snapshot.State);
	}
	Snapshot.Domains.Sort([](const auto& A, const auto& B) { return A.Domain.LexicalLess(B.Domain); });
	return Snapshot;
}

void UCoreKitStateComponent::GetOwnedGameplayTags(FGameplayTagContainer& Tags) const
{
	Tags.Reset();
	for (const auto& Pair : Domains) { Tags.AddTag(Pair.Value.Snapshot.State); }
}

bool UCoreKitStateComponent::IsTagReserved(const FGameplayTag Tag) const
{
	if (!Tag.IsValid()) { return false; }
	for (const auto& Pair : Domains)
	{
		for (const auto& Reserved : Pair.Value.Definition.States)
		{
			if (Tag.MatchesTag(Reserved) || Reserved.MatchesTag(Tag)) { return true; }
		}
	}
	return false;
}

bool UCoreKitStateComponent::IsHealthy(FString& Reason) const
{
	if (bEndingPlay)
	{
		Reason = TEXT("State runtime is ending play.");
		return false;
	}
	if (ProviderQueryDepth > 0)
	{
		Reason = TEXT("A provider query cannot recursively evaluate state health.");
		return false;
	}
	for (const auto& Pair : Domains)
	{
		const auto& Entry = Pair.Value;
		if (!Entry.Writer.IsValid())
		{
			Reason = FString::Printf(
				TEXT("Domain %s lost its authority %s."), *Pair.Key.ToString(), *Entry.Snapshot.AuthorityPath);
			return false;
		}
		if (Entry.Snapshot.bProvider)
		{
			TGuardValue<int32> QueryGuard(ProviderQueryDepth, ProviderQueryDepth + 1);
			const auto State = ICoreKitStateProvider::Execute_QueryDomainState(Entry.Writer.Get(), Pair.Key);
			if (State != Entry.Snapshot.State || !Entry.Definition.States.HasTagExact(State))
			{
				Reason = FString::Printf(
					TEXT("Domain %s provider is unavailable or has not published its state change."),
					*Pair.Key.ToString());
				return false;
			}
		}
	}
	Reason.Reset();
	return true;
}

TArray<FString> UCoreKitStateComponent::GetStateSources(const FGameplayTag Tag) const
{
	TArray<FString> Sources;
	for (const auto& Pair : Domains)
	{
		if (Pair.Value.Snapshot.State.MatchesTag(Tag))
		{
			Sources.Add(FString::Printf(TEXT("%s: %s [domain=%s revision=%lld]"), *Pair.Value.Snapshot.State.ToString(),
			                            *Pair.Value.Snapshot.AuthorityPath, *Pair.Key.ToString(),
			                            Pair.Value.Snapshot.Revision));
		}
	}
	Sources.Sort();
	return Sources;
}

void UCoreKitStateComponent::Record(const FCoreKitStateReport& Report)
{
	const int32 Limit = FMath::Clamp(HistoryLimit, 0, 256);
	if (Limit > 0)
	{
		History.Add(Report);
		if (History.Num() > Limit) { History.RemoveAt(0, History.Num() - Limit); }
	}
	else { History.Reset(); }
	if (bLogTransitions)
	{
		UE_LOG(LogAFStates, Display,
		       TEXT("%s domain=%s %s --%s--> %s result=%s owner=%s execution=%s revision=%lld %s"),
		       *GetNameSafe(GetOwner()), *Report.Domain.ToString(), *Report.From.ToString(), *Report.Event.ToString(),
		       *Report.To.ToString(),
		       *StaticEnum<ECoreKitStateResult>()->GetNameStringByValue(static_cast<int64>(Report.Result)),
		       *Report.AuthorityPath, *Report.ExecutionId.ToString(), Report.SnapshotRevision, *Report.Detail);
	}
}

void UCoreKitStateComponent::Publish(const TArray<FCoreKitStateReport>& Reports, const bool bChanged)
{
	for (const auto& Report : Reports) { Record(Report); }
	// Reentrant requests report failure but cannot recursively notify the same listener.
	if (IsCommittingTransition()) { return; }
	{
		TGuardValue<bool> Guard(bPublishing, true);
		if (bChanged)
		{
			if (BoundActions.IsValid()) { BoundActions->RefreshAuthoritativeStates(); }
			OnSnapshotCommitted.Broadcast(GetStateSnapshot());
		}
		for (const auto& Report : Reports) { OnTransitionReported.Broadcast(Report); }
	}
	if (bChanged) { OnStatesSettled.Broadcast(); }
}
