#include "CoreKitTestFixtures.h"

#include "CoreKitActions/Actions/Animation/CoreKitAnimNotify_IASA.h"
#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	/** Real native montage instances, without game content, a character subclass or a combat component. */
	struct FIASAMontageWorld
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		ACoreKitTestOwner* Owner;
		USkeletalMeshComponent* Mesh;
		UAnimInstance* Animation;
		UCoreKitActionComponent* Actions;

		FIASAMontageWorld()
		{
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			Owner = World->SpawnActor<ACoreKitTestOwner>();
			Mesh = NewObject<USkeletalMeshComponent>(Owner);
			Owner->AddInstanceComponent(Mesh);
			Mesh->RegisterComponent();
			Animation = NewObject<UAnimInstance>(Mesh);
			Mesh->AnimScriptInstance = Animation;
			Actions = NewObject<UCoreKitActionComponent>(Owner);
			Owner->AddInstanceComponent(Actions);
			Actions->ActionSet = NewObject<UCoreKitActionSet>(Actions);
			Actions->RegisterComponent();
			Actions->BeginPlay();
		}

		~FIASAMontageWorld()
		{
			if (Actions->HasBegunPlay())
			{
				Actions->EndPlay(EEndPlayReason::EndPlayInEditor);
			}
			for (FAnimMontageInstance* Instance : Animation->MontageInstances)
			{
				delete Instance;
			}
			Animation->MontageInstances.Reset();
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}

		FAnimMontageInstance* Play(UAnimMontage* Montage = nullptr)
		{
			if (!Montage)
			{
				Montage = NewObject<UAnimMontage>(Owner);
				Montage->SetCompositeLength(2.0f);
			}
			FAnimMontageInstance* Instance = new FAnimMontageInstance(Animation);
			Instance->Initialize(Montage);
			Instance->Play();
			Animation->MontageInstances.Add(Instance);
			return Instance;
		}

		void Notify(FAnimMontageInstance* Instance)
		{
			UCoreKitAnimNotify_IASA* Notify = NewObject<UCoreKitAnimNotify_IASA>();
			FBranchingPointNotifyPayload Payload(Mesh, Instance->Montage, nullptr, Instance->GetInstanceID());
			Notify->BranchingPointNotify(Payload);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASAUnboundMontageTest, "CoreKitActions.IASA.Montages.UnboundNotifies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASAUnboundMontageTest::RunTest(const FString& Parameters)
{
	FIASAMontageWorld Fixture;
	FAnimMontageInstance* Native = Fixture.Play();
	TestEqual(TEXT("An unmarked montage is not interrupted"), Fixture.Actions->InterruptIASAMontages(), 0);
	Fixture.Notify(Native);
	Fixture.Notify(Native);
	TestEqual(TEXT("Native IASA works without an action/animation binding, once per playback"),
		Fixture.Actions->InterruptIASAMontages(), 1);
	TestFalse(TEXT("Its timeline stops immediately"), Native->IsPlaying());
	TestTrue(TEXT("Its pose blends out"), Native->IsStopped());
	FAnimMontageInstance* Stock = Fixture.Play();
	FBranchingPointNotifyPayload Payload(Fixture.Mesh, Stock->Montage, nullptr, Stock->GetInstanceID());
	Fixture.Animation->OnPlayMontageNotifyBegin.Broadcast(TEXT("IASA"), Payload);
	TestEqual(TEXT("A stock montage IASA notify is observed without a combat component"),
		Fixture.Actions->InterruptIASAMontages(), 1);
	TestFalse(TEXT("Stock notify playback stops"), Stock->IsPlaying());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASAMontageReadinessTest, "CoreKitActions.IASA.Montages.ReadinessAndRecommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASAMontageReadinessTest::RunTest(const FString& Parameters)
{
	FIASAMontageWorld Fixture;
	FAnimMontageInstance* Instance = Fixture.Play();
	const FGuid Handle = Fixture.Actions->BlockIASA(Fixture.Owner, TEXT("Montage"));
	TestTrue(TEXT("Bind actual playback"), Fixture.Actions->BindIASAToMontage(Handle, Fixture.Animation, Instance->GetInstanceID()));
	TestEqual(TEXT("Closed IASA cannot cancel"), Fixture.Actions->InterruptIASAMontages(), 0);
	Fixture.Notify(Instance);
	TestTrue(TEXT("Notify releases the playback's block"), Fixture.Actions->IsIASA());
	const FGuid Grab = Fixture.Actions->BlockIASA(Fixture.Mesh, TEXT("Grab"));
	TestEqual(TEXT("A separate control owner still blocks cancellation"), Fixture.Actions->InterruptIASAMontages(), 0);
	Fixture.Actions->ReleaseIASA(Grab);
	const FGuid Recommit = Fixture.Actions->BlockIASA(Fixture.Owner, TEXT("Recommit"));
	Fixture.Actions->BindIASAToMontage(Recommit, Fixture.Animation, Instance->GetInstanceID());
	Fixture.Actions->ReleaseIASABlocks(Fixture.Owner);
	TestEqual(TEXT("Binding a new commitment retires its earlier readiness marker"), Fixture.Actions->InterruptIASAMontages(), 0);
	Fixture.Notify(Instance);
	TestEqual(TEXT("A fresh marker opens it again"), Fixture.Actions->InterruptIASAMontages(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASAMontageReplayTest, "CoreKitActions.IASA.Montages.PlaybackAndOwnerIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASAMontageReplayTest::RunTest(const FString& Parameters)
{
	FIASAMontageWorld Fixture;
	FIASAMontageWorld Other;
	FAnimMontageInstance* Old = Fixture.Play();
	Fixture.Notify(Old);
	Old->Stop(FAlphaBlend(2.0f));
	FAnimMontageInstance* Replay = Fixture.Play(Old->Montage);
	FAnimMontageInstance* Foreign = Other.Play(Old->Montage);
	TestFalse(TEXT("A foreign actor cannot register its animation here"),
		Fixture.Actions->ReleaseIASAForMontage(Other.Animation, Foreign->GetInstanceID()));
	TestEqual(TEXT("Cancel targets the old blending-out instance"), Fixture.Actions->InterruptIASAMontages(), 1);
	TestFalse(TEXT("Old timeline stops"), Old->IsPlaying());
	TestTrue(TEXT("Same asset replay keeps playing before its own notify"), Replay->IsPlaying());
	TestTrue(TEXT("Other actor keeps playing"), Foreign->IsPlaying());
	Fixture.Notify(Replay);
	TestEqual(TEXT("The replay cancels after its own marker"), Fixture.Actions->InterruptIASAMontages(), 1);
	Fixture.Actions->EndPlay(EEndPlayReason::EndPlayInEditor);
	TestEqual(TEXT("Teardown cannot start interruption"), Fixture.Actions->InterruptIASAMontages(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASAMontageReentrancyTest, "CoreKitActions.IASA.Montages.ReentrantPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASAMontageReentrancyTest::RunTest(const FString& Parameters)
{
	FIASAMontageWorld Fixture;
	FAnimMontageInstance* First = Fixture.Play();
	Fixture.Notify(First);
	FAnimMontageInstance* Replacement = nullptr;
	int32 RecursiveResult = INDEX_NONE;
	First->OnMontageBlendingOutStarted.BindLambda([&](UAnimMontage*, bool)
	{
		Replacement = Fixture.Play();
		Fixture.Notify(Replacement);
		RecursiveResult = Fixture.Actions->InterruptIASAMontages();
	});
	TestEqual(TEXT("One snapshot is interrupted"), Fixture.Actions->InterruptIASAMontages(), 1);
	TestEqual(TEXT("Callbacks cannot recursively interrupt new playback"), RecursiveResult, 0);
	if (!TestNotNull(TEXT("Cleanup callback created another playback"), Replacement))
	{
		return false;
	}
	TestTrue(TEXT("Replacement is outside the accepted input snapshot"), Replacement->IsPlaying());
	TestEqual(TEXT("A later accepted input may interrupt replacement"), Fixture.Actions->InterruptIASAMontages(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitIASAMontageActionAdmissionTest, "CoreKitActions.IASA.Montages.ActionAdmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoreKitIASAMontageActionAdmissionTest::RunTest(const FString& Parameters)
{
	FIASAMontageWorld Fixture;
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("CoreKitActions.Tests.Action.A"));
	FCoreKitActionEntry& Entry = Fixture.Actions->ActionSet->Actions.Add(Tag);
	UCoreKitTestAction* Template = NewObject<UCoreKitTestAction>(Fixture.Actions->ActionSet);
	Entry.ActionTemplate = Template;
	Entry.Definition = NewObject<UCoreKitActionDefinition>(Fixture.Actions->ActionSet);
	Entry.Definition->bClosesIASAOnStart = true;
	FAnimMontageInstance* Old = Fixture.Play();
	Fixture.Notify(Old);
	Template->bAllowActivation = false;
	TestEqual(TEXT("Rejected new action"), Fixture.Actions->RequestAction(Tag, FGameplayTag()), ECoreKitActionRequestResult::RejectedByAction);
	TestTrue(TEXT("Rejected action preserves montage"), Old->IsPlaying());
	Template->bAllowActivation = true;
	TestEqual(TEXT("Accepted new action"), Fixture.Actions->RequestAction(Tag, FGameplayTag()), ECoreKitActionRequestResult::Activated);
	TestFalse(TEXT("It interrupts the previous ready montage before activation"), Old->IsPlaying());
	TestFalse(TEXT("The new action retains its own IASA lock"), Fixture.Actions->IsIASA());
	return true;
}
#endif
