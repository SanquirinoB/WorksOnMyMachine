#include "CoreKitTestFixtures.h"

#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/AsyncNodes/CoreKitWaitForActionTag.h"
#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "CoreKitActions/Actions/Types/CoreKitActiveTagLedger.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "CoreKitActions/States/CoreKitStateComponent.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FGameplayTag TestTag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}

	struct FCoreKitTestWorld
	{
		UWorld* World = nullptr;
		ACoreKitTestOwner* Owner = nullptr;
		UCoreKitActionComponent* Actions = nullptr;

		FCoreKitTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			Owner = World->SpawnActor<ACoreKitTestOwner>();
			Actions = NewObject<UCoreKitActionComponent>(Owner);
			Owner->AddInstanceComponent(Actions);
			Actions->ActionSet = NewObject<UCoreKitActionSet>(Actions);
			Actions->RegisterComponent();
			World->BeginPlay();
			if (!Actions->HasBegunPlay())
			{
				Actions->BeginPlay();
			}
		}

		~FCoreKitTestWorld()
		{
			if (Actions && Actions->HasBegunPlay())
			{
				Actions->EndPlay(EEndPlayReason::EndPlayInEditor);
			}
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}

		FCoreKitActionEntry& AddAction(FGameplayTag Tag)
		{
			FCoreKitActionEntry& Entry = Actions->ActionSet->Actions.Add(Tag);
			Entry.ActionTemplate = NewObject<UCoreKitTestAction>(Actions->ActionSet);
			Entry.Definition = NewObject<UCoreKitActionDefinition>(Actions->ActionSet);
			Entry.Definition->RepeatPolicy = ECoreKitActionRepeatPolicy::Reject;
			return Entry;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitLedgerTest, "CoreKitActions.Tags.SourceOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitLedgerTest::RunTest(const FString& Parameters)
{
	FCoreKitActiveTagLedger Ledger;
	const FGameplayTag Tag = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));
	int32 SourceA = 0;
	int32 SourceB = 0;

	TestTrue(TEXT("First source adds the tag"), Ledger.Add(Tag, &SourceA));
	TestFalse(TEXT("The same source cannot add a duplicate claim"), Ledger.Add(Tag, &SourceA));
	TestTrue(TEXT("Second source adds an independent claim"), Ledger.Add(Tag, &SourceB));
	TestTrue(TEXT("Tag remains after one source releases"), Ledger.Remove(Tag, &SourceA));
	TestTrue(TEXT("Second source still owns the tag"), Ledger.HasExact(Tag));
	Ledger.RemoveAllFromSource(&SourceB);
	TestFalse(TEXT("Final release removes the tag"), Ledger.HasExact(Tag));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitActorComponentContextTest, "CoreKitActions.Actions.ActorComponentContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitActorComponentContextTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	TestWorld.AddAction(ActionA);

	UCoreKitTestMarkerComponent* Primary = NewObject<UCoreKitTestMarkerComponent>(TestWorld.Owner, TEXT("PrimaryMarker"));
	TestWorld.Owner->AddInstanceComponent(Primary);
	Primary->RegisterComponent();

	ECoreKitActorComponentLookupResult Result = ECoreKitActorComponentLookupResult::NotFound;
	UCoreKitActionComponent* LazyActions = NewObject<UCoreKitActionComponent>(TestWorld.Owner);
	TestTrue(TEXT("First lookup prepares an unstarted coordinator context"),
		LazyActions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result) == Primary);
	TestEqual(TEXT("Lazy lookup reports Found"), Result, ECoreKitActorComponentLookupResult::Found);
	const int32 LazyRevision = LazyActions->GetActorContextRevision();
	LazyActions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result);
	TestEqual(TEXT("Repeated lookup does not rebuild the snapshot"), LazyActions->GetActorContextRevision(), LazyRevision);

	TestWorld.Actions->RefreshActorContext();
	TestTrue(TEXT("Unique cached component resolves"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result) == Primary);
	TestEqual(TEXT("Unique lookup reports Found"), Result, ECoreKitActorComponentLookupResult::Found);
	TestTrue(TEXT("Unique interface resolves"),
		TestWorld.Actions->GetCachedComponentByInterface(UCoreKitTestMarkerInterface::StaticClass(), Result) == Primary);
	TestEqual(TEXT("Interface lookup reports Found"), Result, ECoreKitActorComponentLookupResult::Found);
	TestNull(TEXT("Invalid component class is rejected"),
		TestWorld.Actions->GetCachedComponentByClass(nullptr, Result));
	TestEqual(TEXT("Invalid class lookup is explicit"), Result, ECoreKitActorComponentLookupResult::InvalidType);
	TestNull(TEXT("Invalid component interface is rejected"),
		TestWorld.Actions->GetCachedComponentByInterface(nullptr, Result));
	TestEqual(TEXT("Invalid interface lookup is explicit"), Result, ECoreKitActorComponentLookupResult::InvalidType);
	const ECoreKitActionRequestResult StartedResult = TestWorld.Actions->RequestAction(ActionA, FGameplayTag());
	TestEqual(TEXT("Action uses the same cached context"), StartedResult, ECoreKitActionRequestResult::Activated);
	UCoreKitAction* ActiveAction = TestWorld.Actions->GetActiveAction(ActionA);
	if (!TestNotNull(TEXT("Action is available after a successful request"), ActiveAction))
	{
		return false;
	}
	TestTrue(TEXT("Action resolves cached component"), ActiveAction->GetCachedOwnerComponentByClass(
		UCoreKitTestMarkerComponent::StaticClass(), Result) == Primary);

	UCoreKitTestMarkerComponent* Secondary = NewObject<UCoreKitTestMarkerComponent>(TestWorld.Owner, TEXT("SecondaryMarker"));
	TestWorld.Owner->AddInstanceComponent(Secondary);
	Secondary->RegisterComponent();
	TestNull(TEXT("New component remains invisible until refresh"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result,
			TEXT("SecondaryMarker")));
	TestEqual(TEXT("Unrefreshed component is not found"), Result, ECoreKitActorComponentLookupResult::NotFound);
	TestWorld.Actions->RefreshActorContext();
	TestNull(TEXT("Ambiguous component type is rejected"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result));
	TestEqual(TEXT("Ambiguity is explicit"), Result, ECoreKitActorComponentLookupResult::Ambiguous);
	TestTrue(TEXT("Component name resolves ambiguity"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result,
			TEXT("PrimaryMarker")) == Primary);
	TestEqual(TEXT("Named lookup reports Found"), Result, ECoreKitActorComponentLookupResult::Found);
	TestNull(TEXT("Ambiguous interface is rejected"),
		TestWorld.Actions->GetCachedComponentByInterface(UCoreKitTestMarkerInterface::StaticClass(), Result));
	TestEqual(TEXT("Interface ambiguity is explicit"), Result, ECoreKitActorComponentLookupResult::Ambiguous);

	Primary->DestroyComponent();
	TestNull(TEXT("Destroyed weak component is never returned"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result,
			TEXT("PrimaryMarker")));
	TestEqual(TEXT("Destroyed component is not found"), Result, ECoreKitActorComponentLookupResult::NotFound);
	TestWorld.Actions->RefreshActorContext();
	TestTrue(TEXT("Refresh resolves the replacement component"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result) == Secondary);
	TestEqual(TEXT("Replacement lookup reports Found"), Result, ECoreKitActorComponentLookupResult::Found);

	TestWorld.Actions->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestNull(TEXT("Teardown makes the context unavailable"),
		TestWorld.Actions->GetCachedComponentByClass(UCoreKitTestMarkerComponent::StaticClass(), Result));
	TestEqual(TEXT("Teardown result is OwnerUnavailable"), Result, ECoreKitActorComponentLookupResult::OwnerUnavailable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitLifecycleTest, "CoreKitActions.Actions.LifecycleAndQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitLifecycleTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	const FGameplayTag ActionB = TestTag(TEXT("CoreKitActions.Tests.Action.B"));
	const FGameplayTag Block = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));

	FCoreKitActionEntry& EntryA = TestWorld.AddAction(ActionA);
	EntryA.Definition->GrantedTagsWhileActive.AddTag(Block);
	CastChecked<UCoreKitTestAction>(EntryA.ActionTemplate)->AuthoredValue = 42;
	FCoreKitActionEntry& EntryB = TestWorld.AddAction(ActionB);
	EntryB.Definition->BlockedTags.AddTag(Block);
	EntryB.Definition->bQueueWhenBlocked = true;

	const FCoreKitActionRequestReport Allowed = TestWorld.Actions->EvaluateAction(ActionA, FGameplayTag());
	TestEqual(TEXT("Policy query allows A"), Allowed.Result, ECoreKitActionRequestResult::Activated);
	TestEqual(TEXT("A query is Allowed, not Started"), Allowed.Status, ECoreKitActionRequestStatus::Allowed);
	TestEqual(TEXT("A starts"), TestWorld.Actions->RequestAction(ActionA, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	TestEqual(TEXT("A request reports Started"), TestWorld.Actions->GetLastRequestReport().Status,
		ECoreKitActionRequestStatus::Started);
	TestEqual(TEXT("Template override is copied"), TestWorld.Owner->CopiedValue, 42);
	TestTrue(TEXT("Execution receives an ID"), TestWorld.Actions->GetLastRequestReport().ExecutionId.IsValid());
	TestEqual(TEXT("B queues while blocked"), TestWorld.Actions->RequestAction(ActionB, FGameplayTag()),
		ECoreKitActionRequestResult::Queued);
	TestEqual(TEXT("B request reports Queued"), TestWorld.Actions->GetLastRequestReport().Status,
		ECoreKitActionRequestStatus::Queued);
	TestWorld.Actions->GetActiveAction(ActionA)->CompleteAction();
	TestNotNull(TEXT("B starts after A cleanup"), TestWorld.Actions->GetActiveAction(ActionB));
	TestEqual(TEXT("A cleanup runs once"), TestWorld.Owner->Cleanups, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitAsyncActionTest, "CoreKitActions.Actions.AsyncAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitAsyncActionTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	const FGameplayTag Block = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));
	FCoreKitActionEntry& Entry = TestWorld.AddAction(ActionA);
	Entry.ActionTemplate = NewObject<UCoreKitTestAsyncAction>(TestWorld.Actions->ActionSet);
	CastChecked<UCoreKitTestAsyncAction>(Entry.ActionTemplate)->bDelayCancellation = true;
	Entry.Definition->GrantedTagsWhileActive.AddTag(Block);

	TestEqual(TEXT("Direct async action starts"), TestWorld.Actions->RequestAction(ActionA, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	UCoreKitTestAsyncAction* Action = CastChecked<UCoreKitTestAsyncAction>(TestWorld.Actions->GetActiveAction(ActionA));
	const FGuid FirstExecution = Action->GetExecutionId();
	TestTrue(TEXT("Execution ID is valid"), FirstExecution.IsValid());
	TestTrue(TEXT("Cancellation request accepted"), TestWorld.Actions->CancelAction(ActionA));
	TestNotNull(TEXT("Action remains registered during asynchronous close"),
		TestWorld.Actions->GetActiveAction(ActionA));
	TestTrue(TEXT("Claims remain during asynchronous close"), TestWorld.Actions->HasActiveTag(Block));
	TestTrue(TEXT("Action accepts its matching completion callback"), Action->CompleteFromCallback(FirstExecution));
	TestNull(TEXT("Action unregisters after cleanup"), TestWorld.Actions->GetActiveAction(ActionA));
	TestFalse(TEXT("Claims release after cleanup"), TestWorld.Actions->HasActiveTag(Block));
	TestEqual(TEXT("Cleanup runs once"), TestWorld.Owner->Cleanups, 1);
	TestEqual(TEXT("Cancellation has the correct final reason"), TestWorld.Owner->LastCleanupReason,
		ECoreKitActionEndReason::Cancelled);

	TestEqual(TEXT("A new execution can start after the first closes"),
		TestWorld.Actions->RequestAction(ActionA, FGameplayTag()), ECoreKitActionRequestResult::Activated);
	UCoreKitTestAsyncAction* Replacement = CastChecked<UCoreKitTestAsyncAction>(TestWorld.Actions->GetActiveAction(ActionA));
	TestTrue(TEXT("Replacement receives a new execution identity"), Replacement->GetExecutionId() != FirstExecution);
	TestFalse(TEXT("Stale callback is rejected while replacement is active"),
		Action->CompleteFromCallback(FirstExecution));
	TestTrue(TEXT("Stale callback leaves replacement active"),
		TestWorld.Actions->GetActiveAction(ActionA) == Replacement);
	Replacement->CompleteAction();

	FCoreKitTestWorld EndPlayWorld;
	FCoreKitActionEntry& EndPlayEntry = EndPlayWorld.AddAction(ActionA);
	EndPlayEntry.ActionTemplate = NewObject<UCoreKitTestAsyncAction>(EndPlayWorld.Actions->ActionSet);
	CastChecked<UCoreKitTestAsyncAction>(EndPlayEntry.ActionTemplate)->bDelayCancellation = true;
	TestEqual(TEXT("Pending end-play action starts"), EndPlayWorld.Actions->RequestAction(ActionA, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	UCoreKitTestAsyncAction* PendingEndPlayAction = CastChecked<UCoreKitTestAsyncAction>(
		EndPlayWorld.Actions->GetActiveAction(ActionA));
	const FGuid PendingEndPlayExecution = PendingEndPlayAction->GetExecutionId();
	TestTrue(TEXT("Pending end-play cancellation is accepted"), EndPlayWorld.Actions->CancelAction(ActionA));
	EndPlayWorld.Actions->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestEqual(TEXT("End-play cleanup has the final reason"), EndPlayWorld.Owner->LastCleanupReason,
		ECoreKitActionEndReason::OwnerEndPlay);
	TestFalse(TEXT("Callback after end play is rejected"),
		PendingEndPlayAction->CompleteFromCallback(PendingEndPlayExecution));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitCommandTest, "CoreKitActions.Actions.AtomicCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitCommandTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	const FGameplayTag Block = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));
	FCoreKitActionEntry& Entry = TestWorld.AddAction(ActionA);
	Entry.ActionTemplate = NewObject<UCoreKitCommandAction>(TestWorld.Actions->ActionSet);
	Entry.Definition->BlockedTags.AddTag(Block);
	int32 DispatchCount = 0;
	const auto Ready = [](FName&, FString&) { return true; };
	const auto Dispatch = [&DispatchCount](FName&, FString&) { ++DispatchCount; return true; };

	FCoreKitActionRequestReport Report = TestWorld.Actions->ExecuteActionCommand(
		ActionA, FGameplayTag(), TestWorld.Actions, Ready, Dispatch);
	TestEqual(TEXT("Command dispatches"), Report.Result, ECoreKitActionRequestResult::Activated);
	TestEqual(TEXT("Successful command reports Executed"), Report.Status, ECoreKitActionRequestStatus::Executed);
	TestEqual(TEXT("Backend runs once"), DispatchCount, 1);
	TestNull(TEXT("Command creates no task"), TestWorld.Actions->GetActiveAction(ActionA));
	TestWorld.Actions->SetTagClaim(TestWorld.Actions, Block, true);
	Report = TestWorld.Actions->ExecuteActionCommand(ActionA, FGameplayTag(), TestWorld.Actions, Ready, Dispatch);
	TestEqual(TEXT("Block rejects command"), Report.Result, ECoreKitActionRequestResult::BlockedByTags);
	TestEqual(TEXT("Blocked command reports Rejected"), Report.Status, ECoreKitActionRequestStatus::Rejected);
	TestEqual(TEXT("Rejected command has no effects"), DispatchCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitRequestIdentityTest, "CoreKitActions.Actions.RequestIdentityAndContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitRequestIdentityTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	TestWorld.AddAction(ActionA);
	UCoreKitTestContext* Context = NewObject<UCoreKitTestContext>(TestWorld.Actions);

	const FCoreKitActionRequestReport Started = TestWorld.Actions->RequestActionWithContext(
		ActionA, FGameplayTag(), Context);
	TestEqual(TEXT("Context request starts"), Started.Result, ECoreKitActionRequestResult::Activated);
	TestTrue(TEXT("Request ID is valid"), Started.RequestId.IsValid());
	TestTrue(TEXT("Execution ID is valid"), Started.ExecutionId.IsValid());
	TestTrue(TEXT("Action retains exact context"),
		TestWorld.Actions->GetActiveAction(ActionA)->GetActionContext() == Context);
	TestFalse(TEXT("Unknown execution cannot be cancelled"),
		TestWorld.Actions->CancelExecution(FGuid::NewGuid()));
	TestTrue(TEXT("Exact execution can be cancelled"),
		TestWorld.Actions->CancelExecution(Started.ExecutionId));

	Context->bValid = false;
	const FCoreKitActionRequestReport Rejected = TestWorld.Actions->RequestActionWithContext(
		ActionA, FGameplayTag(), Context);
	TestEqual(TEXT("Invalid context is a permanent rejection"), Rejected.Result,
		ECoreKitActionRequestResult::InvalidContext);
	TestEqual(TEXT("Invalid context cannot report Allowed"), Rejected.Status, ECoreKitActionRequestStatus::Rejected);
	TestTrue(TEXT("Rejected request still has an ID"), Rejected.RequestId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitQueueIdentityTest, "CoreKitActions.Actions.QueueIdentityPriorityAndExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitQueueIdentityTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	const FGameplayTag ActionB = TestTag(TEXT("CoreKitActions.Tests.Action.B"));
	const FGameplayTag Block = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));
	FCoreKitActionEntry& High = TestWorld.AddAction(ActionA);
	High.Definition->BlockedTags.AddTag(Block);
	High.Definition->bQueueWhenBlocked = true;
	High.Definition->QueuePriority = 10;
	High.Definition->QueueWindowSeconds = 0.01f;
	FCoreKitActionEntry& Low = TestWorld.AddAction(ActionB);
	Low.Definition->BlockedTags.AddTag(Block);
	Low.Definition->bQueueWhenBlocked = true;
	Low.Definition->QueuePriority = 1;
	TestWorld.Actions->SetTagClaim(TestWorld.Actions, Block, true);

	const FCoreKitActionRequestReport Queued = TestWorld.Actions->RequestActionWithContext(
		ActionA, FGameplayTag(), nullptr);
	TestEqual(TEXT("High-priority request queues"), Queued.Result, ECoreKitActionRequestResult::Queued);
	TestEqual(TEXT("Component exposes queued identity"), TestWorld.Actions->GetQueuedRequestId(), Queued.RequestId);
	const FCoreKitActionRequestReport Rejected = TestWorld.Actions->RequestActionWithContext(
		ActionB, FGameplayTag(), nullptr);
	TestEqual(TEXT("Lower-priority request reports truthful rejection"), Rejected.Result,
		ECoreKitActionRequestResult::QueueRejected);
	TestEqual(TEXT("Queue refusal reports Rejected"), Rejected.Status, ECoreKitActionRequestStatus::Rejected);
	TestEqual(TEXT("Rejected request does not replace queue owner"),
		TestWorld.Actions->GetQueuedRequestId(), Queued.RequestId);
	TestFalse(TEXT("Wrong request cannot clear queue"),
		TestWorld.Actions->CancelQueuedRequest(Rejected.RequestId));

	TestWorld.World->Tick(LEVELTICK_All, 0.05f);
	TestWorld.Actions->SetTagClaim(TestWorld.Actions, Block, false);
	TestFalse(TEXT("Expired request does not start"), TestWorld.Actions->GetActiveAction(ActionA) != nullptr);
	TestEqual(TEXT("Expiry is observable"), TestWorld.Actions->GetLastRequestReport().Result,
		ECoreKitActionRequestResult::RequestExpired);
	TestFalse(TEXT("Expiry clears queue identity"), TestWorld.Actions->GetQueuedRequestId().IsValid());
	TestWorld.Actions->SetTagClaim(TestWorld.Actions, Block, true);
	const FCoreKitActionRequestReport LowAfterExpiry = TestWorld.Actions->RequestActionWithContext(
		ActionB, FGameplayTag(), nullptr);
	TestEqual(TEXT("Expired queue no longer rejects a lower priority request"), LowAfterExpiry.Result,
		ECoreKitActionRequestResult::Queued);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitReentryAndIdentityTest, "CoreKitActions.Actions.ReentryAndExecutionIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitReentryAndIdentityTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	const FGameplayTag ActionB = TestTag(TEXT("CoreKitActions.Tests.Action.B"));
	const FGameplayTag Block = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));
	FCoreKitActionEntry& A = TestWorld.AddAction(ActionA);
	A.Definition->GrantedTagsWhileActive.AddTag(Block);
	FCoreKitActionEntry& B = TestWorld.AddAction(ActionB);
	B.Definition->bQueueWhenBlocked = true;
	CastChecked<UCoreKitTestAction>(B.ActionTemplate)->bAllowActivation = false;
	const FCoreKitActionRequestReport QueuedB = TestWorld.Actions->RequestActionWithContext(ActionB, FGameplayTag(), nullptr);
	TestEqual(TEXT("Unavailable B queues"), QueuedB.Result, ECoreKitActionRequestResult::Queued);
	CastChecked<UCoreKitTestAction>(B.ActionTemplate)->bAllowActivation = true;
	const FCoreKitActionRequestReport StartedA = TestWorld.Actions->RequestActionWithContext(ActionA, FGameplayTag(), nullptr);
	UCoreKitAction* ActiveA = TestWorld.Actions->GetActiveAction(ActionA);
	UCoreKitAction* ActiveB = TestWorld.Actions->GetActiveAction(ActionB);
	TestNotNull(TEXT("A starts"), ActiveA);
	TestNotNull(TEXT("Queued B starts after A request"), ActiveB);
	TestEqual(TEXT("A report retains A's execution identity"), StartedA.ExecutionId, ActiveA->GetExecutionId());
	TestFalse(TEXT("A report is not overwritten by B's start"), StartedA.ExecutionId == ActiveB->GetExecutionId());

	FCoreKitTestWorld ReentryWorld;
	FCoreKitActionEntry& Reentry = ReentryWorld.AddAction(ActionA);
	Reentry.Definition->GrantedTagsWhileActive.AddTag(Block);
	UCoreKitTestTagListener* Listener = NewObject<UCoreKitTestTagListener>(ReentryWorld.Actions);
	Listener->Actions = ReentryWorld.Actions;
	Listener->FinishWhenAdded = ActionA;
	Listener->ActionToFinish = ActionA;
	ReentryWorld.Actions->OnActiveTagAdded.AddDynamic(Listener, &UCoreKitTestTagListener::HandleTagAdded);
	TestEqual(TEXT("Reentrant action starts then completes"), ReentryWorld.Actions->RequestAction(ActionA, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	TestNull(TEXT("Reentrant completion unregisters action"), ReentryWorld.Actions->GetActiveAction(ActionA));
	TestFalse(TEXT("No granted claim survives reentrant finish"), ReentryWorld.Actions->HasActiveTag(Block));
	TestEqual(TEXT("Reentrant cleanup remains unique"), ReentryWorld.Owner->Cleanups, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitManagedStateTest, "CoreKitActions.States.ManagedAuthorityAndAtomicBatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitManagedStateTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	UCoreKitStateComponent* States = NewObject<UCoreKitStateComponent>(TestWorld.Owner);
	TestWorld.Owner->AddInstanceComponent(States);
	States->RegisterComponent();
	if (!States->HasBegunPlay())
	{
		States->BeginPlay();
	}

	const FGameplayTag Idle = TestTag(TEXT("State.CoreKitActions.Tests.Mode.Idle"));
	const FGameplayTag Active = TestTag(TEXT("State.CoreKitActions.Tests.Mode.Active"));
	const FGameplayTag Calm = TestTag(TEXT("State.CoreKitActions.Tests.Alert.Calm"));
	const FGameplayTag Alerted = TestTag(TEXT("State.CoreKitActions.Tests.Alert.Alerted"));
	const FGameplayTag ActivateEvent = TestTag(TEXT("Event.CoreKitActions.Tests.Activate"));
	const FGameplayTag AlertEvent = TestTag(TEXT("Event.CoreKitActions.Tests.Alert"));
	FCoreKitStateDomainDefinition Mode;
	Mode.Domain = TEXT("Mode");
	Mode.States.AddTag(Idle);
	Mode.States.AddTag(Active);
	Mode.InitialState = Idle;
	Mode.Transitions.Add({Idle, ActivateEvent, Active});
	FCoreKitStateDomainDefinition Alert;
	Alert.Domain = TEXT("Alert");
	Alert.States.AddTag(Calm);
	Alert.States.AddTag(Alerted);
	Alert.InitialState = Calm;
	Alert.Transitions.Add({Calm, AlertEvent, Alerted});
	FCoreKitStateAuthority ModeAuthority;
	FCoreKitStateAuthority AlertAuthority;
	TestEqual(TEXT("Mode domain registers"),
		States->RegisterDomain(Mode, TestWorld.Owner, ModeAuthority).Result, ECoreKitStateResult::Success);
	TestEqual(TEXT("Alert domain registers"),
		States->RegisterDomain(Alert, TestWorld.Owner, AlertAuthority).Result, ECoreKitStateResult::Success);
	TestTrue(TEXT("Authoritative state is projected to actions"), TestWorld.Actions->HasActiveTag(Idle));

	FCoreKitStateTransitionRequest ModeRequest;
	ModeRequest.Authority = ModeAuthority;
	ModeRequest.Writer = TestWorld.Owner;
	ModeRequest.ExpectedState = Idle;
	ModeRequest.ExpectedRevision = 1;
	ModeRequest.Event = ActivateEvent;
	ModeRequest.To = Active;
	ModeRequest.ExecutionId = FGuid::NewGuid();
	FCoreKitStateTransitionRequest AlertRequest;
	AlertRequest.Authority = AlertAuthority;
	AlertRequest.Writer = TestWorld.Owner;
	AlertRequest.ExpectedState = Calm;
	AlertRequest.ExpectedRevision = 99;
	AlertRequest.Event = AlertEvent;
	AlertRequest.To = Alerted;
	AlertRequest.ExecutionId = FGuid::NewGuid();
	TestEqual(TEXT("One stale member rejects the whole batch"),
		States->TryTransitionBatch({ModeRequest, AlertRequest}).Result, ECoreKitStateResult::StaleRevision);
	TestEqual(TEXT("Rejected batch leaves first domain unchanged"),
		States->GetDomainState(TEXT("Mode")).State, Idle);
	AlertRequest.ExpectedRevision = 1;
	TestEqual(TEXT("Valid batch commits"),
		States->TryTransitionBatch({ModeRequest, AlertRequest}).Result, ECoreKitStateResult::Success);
	TestEqual(TEXT("Mode commits atomically"), States->GetDomainState(TEXT("Mode")).State, Active);
	TestEqual(TEXT("Alert commits atomically"), States->GetDomainState(TEXT("Alert")).State, Alerted);

	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	FCoreKitActionEntry& Entry = TestWorld.AddAction(ActionA);
	Entry.Definition->GrantedTagsWhileActive.AddTag(Active);
	const FCoreKitActionRequestReport ReservedWrite = TestWorld.Actions->EvaluateAction(ActionA, FGameplayTag());
	TestEqual(TEXT("Actions cannot write an authoritative state tag"), ReservedWrite.Result,
		ECoreKitActionRequestResult::InvalidConfiguration);
	TestEqual(TEXT("Reserved write has stable diagnostic"), ReservedWrite.FailureCode,
		FName(TEXT("ReservedStateMutation")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitProvidedStateTest, "CoreKitActions.States.ProviderHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitProvidedStateTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	UCoreKitStateComponent* States = NewObject<UCoreKitStateComponent>(TestWorld.Owner);
	TestWorld.Owner->AddInstanceComponent(States);
	States->RegisterComponent();
	if (!States->HasBegunPlay())
	{
		States->BeginPlay();
	}
	UCoreKitTestStateProvider* Provider = NewObject<UCoreKitTestStateProvider>(TestWorld.Owner);
	const FGameplayTag Idle = TestTag(TEXT("State.CoreKitActions.Tests.Mode.Idle"));
	const FGameplayTag Active = TestTag(TEXT("State.CoreKitActions.Tests.Mode.Active"));
	Provider->CurrentState = Idle;
	FCoreKitStateDomainDefinition Domain;
	Domain.Domain = TEXT("ExternalMode");
	Domain.States.AddTag(Idle);
	Domain.States.AddTag(Active);
	Domain.InitialState = Idle;
	FCoreKitStateAuthority Authority;
	TestEqual(TEXT("Read-only provider registers"),
		States->RegisterProvidedDomain(Domain, Provider, Authority).Result, ECoreKitStateResult::Success);
	FString Reason;
	TestTrue(TEXT("Published provider state is healthy"), States->IsHealthy(Reason));
	Provider->CurrentState = Active;
	TestFalse(TEXT("Unpublished provider mutation fails closed"), States->IsHealthy(Reason));
	TestEqual(TEXT("Provider can publish its new state"),
		States->RefreshProvidedDomain(Authority, Provider).Result, ECoreKitStateResult::Success);
	TestTrue(TEXT("Refresh restores health"), States->IsHealthy(Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitWaitNodeTest, "CoreKitActions.Actions.WaitForTagLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitWaitNodeTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag Block = TestTag(TEXT("CoreKitActions.Tests.State.Blocked"));
	UCoreKitWaitForActionTag* Completed = UCoreKitWaitForActionTag::WaitForActionTag(TestWorld.Owner, Block);
	Completed->Activate();
	TestWorld.Actions->SetTagClaim(TestWorld.Actions, Block, true);
	TestEqual(TEXT("Tag notification completes node"), Completed->GetResult(),
		ECoreKitWaitForActionTagResult::Completed);

	TestWorld.Actions->SetTagClaim(TestWorld.Actions, Block, false);
	UCoreKitWaitForActionTag* Cancelled = UCoreKitWaitForActionTag::WaitForActionTag(TestWorld.Owner, Block);
	Cancelled->Activate();
	Cancelled->Cancel();
	TestEqual(TEXT("Explicit cancellation is observable"), Cancelled->GetResult(),
		ECoreKitWaitForActionTagResult::Cancelled);

	UCoreKitWaitForActionTag* TimedOut = UCoreKitWaitForActionTag::WaitForActionTag(TestWorld.Owner, Block, 0.01f);
	TimedOut->Activate();
	// Automation runs several tests in one engine frame. Advance the frame counter so the
	// TimerManager accepts two synthetic ticks: one activates the pending timer, one fires it.
	++GFrameCounter;
	TestWorld.World->GetTimerManager().Tick(0.05f);
	++GFrameCounter;
	TestWorld.World->GetTimerManager().Tick(0.05f);
	TestEqual(TEXT("Timeout is observable"), TimedOut->GetResult(), ECoreKitWaitForActionTagResult::TimedOut);

	UCoreKitWaitForActionTag* Invalid = UCoreKitWaitForActionTag::WaitForActionTag(nullptr, Block);
	Invalid->Activate();
	TestEqual(TEXT("Invalid source completes explicitly"), Invalid->GetResult(),
		ECoreKitWaitForActionTagResult::InvalidSource);

	UCoreKitWaitForActionTag* ComponentEnded = UCoreKitWaitForActionTag::WaitForActionTag(TestWorld.Owner, Block);
	ComponentEnded->Activate();
	TestWorld.Actions->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestEqual(TEXT("Component teardown completes explicitly"), ComponentEnded->GetResult(),
		ECoreKitWaitForActionTagResult::ComponentEnded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitTickOptInTest, "CoreKitActions.Actions.TickIsOptIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitTickOptInTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionA = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	FCoreKitActionEntry& Entry = TestWorld.AddAction(ActionA);
	Entry.ActionTemplate = NewObject<UCoreKitTestTickAction>(TestWorld.Actions->ActionSet);
	Entry.Definition->bTickAction = false;
	TestEqual(TEXT("Non-ticking action starts"), TestWorld.Actions->RequestAction(ActionA, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	TestWorld.Actions->TickComponent(0.05f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Tick stays dormant by default"), TestWorld.Owner->Ticks, 0);
	TestWorld.Actions->GetActiveAction(ActionA)->CompleteAction();

	Entry.Definition->bTickAction = true;
	TestEqual(TEXT("Ticking action starts"), TestWorld.Actions->RequestAction(ActionA, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	TestWorld.Actions->TickComponent(0.05f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Opt-in action receives task ticks"), TestWorld.Owner->Ticks > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitGlobalIASATest, "CoreKitActions.IASA.GlobalOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitGlobalIASATest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	UCoreKitActionComponent* Actions = TestWorld.Actions;
	UObject* Attack = NewObject<UCoreKitTestMarkerComponent>(TestWorld.Owner);
	UObject* Grab = NewObject<UCoreKitTestMarkerComponent>(TestWorld.Owner);
	TestTrue(TEXT("Idle is globally ready"), Actions->IsIASA());
	TestTrue(TEXT("Idle publishes the neutral tag"), Actions->HasActiveTag(CoreKitActionTags::IASA));
	FString ReservationReason;
	TestFalse(TEXT("A state domain cannot take ownership of derived readiness"),
		Actions->CanReserveStateTags(FGameplayTagContainer(CoreKitActionTags::IASA), ReservationReason));
	const FGuid AttackHandle = Actions->BlockIASA(Attack, TEXT("Attack"));
	TestFalse(TEXT("A committed action closes global readiness"), Actions->IsIASA());
	Actions->SetIASAStateBlock(Grab, TEXT("Grab"), true);
	Actions->SetIASAStateBlock(Grab, TEXT("Grab"), true);
	TestTrue(TEXT("The retired attack releases only its own handle"), Actions->ReleaseIASA(AttackHandle));
	TestFalse(TEXT("Ending an old montage cannot open an active grab"), Actions->IsIASA());
	TestFalse(TEXT("An old handle cannot release twice"), Actions->ReleaseIASA(AttackHandle));
	Actions->SetTagClaim(Attack, CoreKitActionTags::IASA, true);
	TestFalse(TEXT("A tag claim cannot counterfeit readiness"), Actions->HasActiveTag(CoreKitActionTags::IASA));
	Actions->SetIASAStateBlock(Grab, TEXT("Grab"), false);
	TestTrue(TEXT("Ending the grab restores readiness with no duplicate block"), Actions->IsIASA());
	TestTrue(TEXT("Restoration publishes the global tag"), Actions->HasActiveTag(CoreKitActionTags::IASA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASAActionLifetimeTest, "CoreKitActions.IASA.ActionLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASAActionLifetimeTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag ActionTag = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	FCoreKitActionEntry& Entry = TestWorld.AddAction(ActionTag);
	Entry.Definition->bClosesIASAOnStart = true;
	TestEqual(TEXT("A ready actor starts the action"), TestWorld.Actions->RequestAction(ActionTag, FGameplayTag()),
		ECoreKitActionRequestResult::Activated);
	UCoreKitAction* Action = TestWorld.Actions->GetActiveAction(ActionTag);
	TestNotNull(TEXT("The action is retained"), Action);
	if (!Action)
	{
		return false;
	}
	TestFalse(TEXT("The action closes global IASA on start"), TestWorld.Actions->IsIASA());
	Action->SetIASA(true);
	TestTrue(TEXT("IASA can reopen before the action finishes"), TestWorld.Actions->IsIASA());
	TestTrue(TEXT("Opening IASA does not finish the action"), TestWorld.Actions->GetActiveAction(ActionTag) == Action);
	Action->SetIASA(false);
	Action->CompleteAction();
	TestTrue(TEXT("Completion releases the execution's remaining block"), TestWorld.Actions->IsIASA());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASACommandTest, "CoreKitActions.IASA.CommandsAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASACommandTest::RunTest(const FString& Parameters)
{
	FCoreKitTestWorld TestWorld;
	const FGameplayTag CommandTag = TestTag(TEXT("CoreKitActions.Tests.Action.A"));
	FCoreKitActionEntry& Entry = TestWorld.AddAction(CommandTag);
	Entry.ActionTemplate = NewObject<UCoreKitCommandAction>(TestWorld.Actions->ActionSet);
	const FGuid Handle = TestWorld.Actions->BlockIASA(TestWorld.Owner, TEXT("Grab"));
	int32 DispatchCount = 0;
	const auto Ready = [](FName&, FString&) { return true; };
	const auto Dispatch = [&DispatchCount](FName&, FString&)
	{
		++DispatchCount;
		return true;
	};
	FCoreKitActionRequestReport Report = TestWorld.Actions->ExecuteActionCommand(CommandTag, FGameplayTag(),
		TestWorld.Actions, Ready, Dispatch);
	TestEqual(TEXT("New voluntary gameplay is rejected"), Report.FailureCode, FName(TEXT("IASAClosed")));
	TestEqual(TEXT("The blocked backend never executes"), DispatchCount, 0);
	Report = TestWorld.Actions->ExecuteActionCommand(CommandTag, FGameplayTag(),
		TestWorld.Actions, Ready, Dispatch, true);
	TestEqual(TEXT("Release/cleanup can progress while blocked"), DispatchCount, 1);
	TestFalse(TEXT("Cleanup does not itself open the grab"), TestWorld.Actions->IsIASA());
	TestWorld.Actions->ReleaseIASA(Handle);
	TestWorld.Actions->ExecuteActionCommand(CommandTag, FGameplayTag(), TestWorld.Actions, Ready, Dispatch);
	TestEqual(TEXT("Gameplay resumes after readiness is restored"), DispatchCount, 2);
	return true;
}

#endif
