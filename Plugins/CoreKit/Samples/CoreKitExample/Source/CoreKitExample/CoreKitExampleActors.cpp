#include "CoreKitExampleActors.h"

#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "CoreKitExampleGame.h"
#include "CoreKitExampleHeldIntent.h"
#include "CoreKitTween/CoreKitTweenSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(DemoBlock, "Action.CoreKitExample.Block");

ACoreKitExampleDoor::ACoreKitExampleDoor()
{
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Hinge);
	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
	Panel->SetupAttachment(Hinge);
	Panel->SetRelativeLocation(FVector(0, 65, 105));
	Panel->SetRelativeScale3D(FVector(.14, 1.3, 2.1));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Panel->SetStaticMesh(Cube.Object);
	Persistence = CreateDefaultSubobject<UCoreKitExampleDoorSave>(TEXT("Persistence"));
}

void ACoreKitExampleDoor::SetOpen(bool bNewOpen, bool bAnimate)
{
	bOpen = bNewOpen;
	const float End = bOpen ? OpenAngle : 0.0f;
	const float Start = Hinge->GetRelativeRotation().Yaw;
	auto* Tween = GetWorld()->GetSubsystem<UCoreKitTweenSubsystem>();
	const auto* Demo = GetGameInstance<UCoreKitExampleGame>();
	const float Duration = Demo ? Demo->GetDoorDuration() : .45f;
	if (!bAnimate || !Tween)
	{
		if (Tween)
		{
			Tween->CancelTweensByChannel(this, TEXT("Door.Motion"));
		}
		Hinge->SetRelativeRotation(FRotator(0, End, 0));
		return;
	}

	FCoreKitTweenOptions Options;
	Options.Channel = TEXT("Door.Motion");
	Options.Conflict = ECoreKitTweenConflict::Replace;
	Tween->StartSequence(this, {{Duration, ECoreKitTweenEase::SmoothStep}}, Options,
		FCoreKitTweenStepUpdated::CreateWeakLambda(this, [this, Start, End](int32, float Alpha)
		{
			Hinge->SetRelativeRotation(FRotator(0, FMath::Lerp(Start, End, Alpha), 0));
		}));
}

bool UCoreKitExampleDoorSave::CaptureSaveState_Implementation(FCoreKitParticipantState& OutState)
{
	const auto* Door = CastChecked<ACoreKitExampleDoor>(GetOwner());
	OutState.Payload.InitializeAs<FCoreKitExampleDoorState>();
	OutState.Payload.GetMutable<FCoreKitExampleDoorState>().bOpen = Door->IsOpen();
	return true;
}

bool UCoreKitExampleDoorSave::CanRestoreSaveState_Implementation(const FCoreKitParticipantState& State)
{
	return State.Version == 1 && State.Payload.GetPtr<FCoreKitExampleDoorState>();
}

void UCoreKitExampleDoorSave::ApplySaveState_Implementation(const FCoreKitParticipantState& State)
{
	CastChecked<ACoreKitExampleDoor>(GetOwner())->SetOpen(State.Payload.Get<FCoreKitExampleDoorState>().bOpen, false);
}

ACoreKitExampleController::ACoreKitExampleController()
{
	Actions = CreateDefaultSubobject<UCoreKitActionComponent>(TEXT("Actions"));
	Held = CreateDefaultSubobject<UCoreKitExampleHeldIntent>(TEXT("HeldIntent"));
}

void ACoreKitExampleController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		auto* Demo = GetGameInstance<UCoreKitExampleGame>();
		if (const auto* Door = Demo->GetDoorInput())
		{
			Input->BindAction(Door, ETriggerEvent::Started, this, &ThisClass::ToggleDoor);
		}
		if (const auto* Block = Demo->GetBlockInput())
		{
			Input->BindAction(Block, ETriggerEvent::Started, this, &ThisClass::StartBlock);
			Input->BindAction(Block, ETriggerEvent::Completed, this, &ThisClass::StopBlock);
			Input->BindAction(Block, ETriggerEvent::Canceled, this, &ThisClass::StopBlock);
		}
	}
}

void ACoreKitExampleController::BeginPlay()
{
	Super::BeginPlay();
	auto* Demo = GetGameInstance<UCoreKitExampleGame>();
	Actions->ActionSet = NewObject<UCoreKitActionSet>(Actions);
	auto& Entry = Actions->ActionSet->Actions.Add(DemoBlock);
	Entry.Definition = NewObject<UCoreKitActionDefinition>(Actions->ActionSet);
	Entry.ActionTemplate = NewObject<UCoreKitAction>(Actions->ActionSet, Demo->GetBlockActionClass());
	Held->Configure(Actions, DemoBlock);
	Actions->OnActionStarted.AddDynamic(this, &ThisClass::BlockStarted);
	Actions->OnActionFinished.AddDynamic(this, &ThisClass::BlockFinished);
}

void ACoreKitExampleController::ToggleDoor()
{
	GetGameInstance<UCoreKitExampleGame>()->ToggleDoor();
}

void ACoreKitExampleController::SetBlockHeld(bool bHeld)
{
	Held->SetHeld(bHeld);
}

void ACoreKitExampleController::StartBlock() { SetBlockHeld(true); }
void ACoreKitExampleController::StopBlock() { SetBlockHeld(false); }

bool ACoreKitExampleController::IsBlocking() const
{
	return Actions->GetActiveAction(DemoBlock) != nullptr;
}

void ACoreKitExampleController::BlockStarted(FGameplayTag Tag, UCoreKitAction*)
{
	if (Tag == DemoBlock)
	{
		GetGameInstance<UCoreKitExampleGame>()->ShowBlockStatus();
	}
}

void ACoreKitExampleController::BlockFinished(FGameplayTag Tag, UCoreKitAction*, ECoreKitActionEndReason)
{
	if (Tag == DemoBlock)
	{
		GetGameInstance<UCoreKitExampleGame>()->ShowBlockStatus();
	}
}

void ACoreKitExampleController::SimulateBusy()
{
	if (BusyId.IsValid())
	{
		return;
	}
	BusyId = Actions->BlockIASA(this, TEXT("ExampleRecovery"));
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this]
	{
		const FGuid Finished = BusyId;
		BusyId.Invalidate();
		Actions->ReleaseIASA(Finished);
	}), 2.0f, false);
}

ACoreKitExampleGameMode::ACoreKitExampleGameMode()
{
	PlayerControllerClass = ACoreKitExampleController::StaticClass();
	DefaultPawnClass = nullptr;
}
