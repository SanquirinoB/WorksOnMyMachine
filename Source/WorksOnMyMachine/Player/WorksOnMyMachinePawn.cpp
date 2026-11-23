#include "WorksOnMyMachinePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "WorksOnMyMachineMoverComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "MoveLibrary/ConstrainedMoveUtils.h"
#include "MoverDataModelTypes.h"
#include "UObject/ConstructorHelpers.h"

AWorksOnMyMachinePawn::AWorksOnMyMachinePawn()
{
    bReplicates = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    SetRootComponent(Capsule);
    Capsule->InitCapsuleSize(34.f, 88.f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(Capsule);
    PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderMesh->SetCanEverAffectNavigation(false);
    PlaceholderMesh->SetRelativeScale3D(FVector(.45, .45, 1.5));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    PlaceholderMesh->SetStaticMesh(Cube.Object);

    Mover = CreateDefaultSubobject<UWorksOnMyMachineMoverComponent>(TEXT("Mover"));

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Capsule);
    Camera->SetAbsolute(true, true, false);
    Camera->SetWorldRotation(FRotator(-5., -90., 0.));
    Camera->SetFieldOfView(60.f);
}

void AWorksOnMyMachinePawn::BeginPlay()
{
    Super::BeginPlay();

    if (UCommonLegacyMovementSettings* Settings = Mover->FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
    {
        Settings->MaxSpeed = MaxGroundSpeed;
        Settings->JumpUpwardsSpeed = JumpSpeed;
    }

    Mover->SetGravityOverride(true, FVector::UpVector * (GetWorld()->GetGravityZ() * GravityScale));
    FPlanarConstraint Constraint;
    Constraint.bConstrainToPlane = true;
    Constraint.PlaneConstraintNormal = FVector::RightVector;
    Constraint.PlaneConstraintOrigin = GetActorLocation();
    Mover->SetPlanarConstraint(Constraint);
    Camera->SetWorldLocation(GetActorLocation() + FVector(0., 700., 65.));
    Mover->OnPostFinalize.AddUniqueDynamic(this, &ThisClass::UpdateCameraPosition);
}

void AWorksOnMyMachinePawn::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearInput();
    Super::EndPlay(Reason);
}

void AWorksOnMyMachinePawn::UnPossessed()
{
    ClearInput();
    Super::UnPossessed();
}

FVector AWorksOnMyMachinePawn::GetVelocity() const
{
    return Mover ? Mover->GetVelocity() : FVector::ZeroVector;
}

void AWorksOnMyMachinePawn::ProduceInput_Implementation(int32 /*SimTimeMs*/, FMoverInputCmdContext& InputCmd)
{
    FCharacterDefaultInputs& Inputs = InputCmd.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
    const FVector Direction = FVector::ForwardVector * FMath::Clamp(HorizontalIntent, -1.f, 1.f);
    Inputs.SetMoveInput(EMoveInputType::DirectionalIntent, Direction);
    Inputs.OrientationIntent = Direction.GetSafeNormal();
    Inputs.ControlRotation = GetControlRotation();
    Inputs.bIsJumpPressed = bJumpPressed;
    Inputs.bIsJumpJustPressed = bJumpRequested;
    bJumpRequested = false;
}

void AWorksOnMyMachinePawn::UpdateCameraPosition(const FMoverSyncState& /*SyncState*/, const FMoverAuxStateContext& /*AuxState*/)
{
    Camera->SetWorldLocation(GetActorLocation() + FVector(0., 700., 65.));
}

void AWorksOnMyMachinePawn::CreateInputMappings()
{
    InputContext = NewObject<UInputMappingContext>(this);
    MoveAction = NewObject<UInputAction>(this);
    MoveAction->ValueType = EInputActionValueType::Axis1D;
    MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
    JumpAction = NewObject<UInputAction>(this);

    InputContext->MapKey(MoveAction, EKeys::D);
    InputContext->MapKey(MoveAction, EKeys::Right);
    InputContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
    InputContext->MapKey(MoveAction, EKeys::Left).Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
    InputContext->MapKey(MoveAction, EKeys::Gamepad_LeftX).Modifiers.Add(NewObject<UInputModifierDeadZone>(InputContext));
    InputContext->MapKey(JumpAction, EKeys::SpaceBar);
    InputContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
}

void AWorksOnMyMachinePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    APlayerController* Player = Cast<APlayerController>(GetController());
    if (!Input || !Player || !Player->GetLocalPlayer())
    {
        return;
    }

    ClearInput();
    if (!InputContext)
    {
        CreateInputMappings();
    }
    InputSubsystem = Player->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (InputSubsystem.IsValid())
    {
        InputSubsystem->AddMappingContext(InputContext, 0);
    }

    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
    Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopMoving);
    Input->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::StopMoving);
    Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJump);
    Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
    Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::StopJump);
}

void AWorksOnMyMachinePawn::ClearInput()
{
    if (InputSubsystem.IsValid() && InputContext)
    {
        InputSubsystem->RemoveMappingContext(InputContext);
    }
    InputSubsystem.Reset();
    StopMoving();
    StopJump();
    bJumpRequested = false;
}

void AWorksOnMyMachinePawn::Move(const FInputActionValue& Value) { HorizontalIntent = Value.Get<float>(); }
void AWorksOnMyMachinePawn::StopMoving() { HorizontalIntent = 0.f; }

void AWorksOnMyMachinePawn::StartJump()
{
    bJumpPressed = true;
    bJumpRequested = true;
}

void AWorksOnMyMachinePawn::StopJump() { bJumpPressed = false; }
