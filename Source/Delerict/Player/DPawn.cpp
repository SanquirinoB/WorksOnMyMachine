#include "DPawn.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/CameraAsset.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "DLocomotionAnimInstance.h"
#include "DMoverComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "MoveLibrary/ConstrainedMoveUtils.h"
#include "MoverDataModelTypes.h"
#include "UObject/ConstructorHelpers.h"

ADPawn::ADPawn()
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

    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Capsule);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetAnimInstanceClass(UDLocomotionAnimInstance::StaticClass());

    Mover = CreateDefaultSubobject<UDMoverComponent>(TEXT("Mover"));

    GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));
    GameplayCamera->SetupAttachment(Capsule);
    GameplayCamera->SetUsingAbsoluteRotation(true);
    GameplayCamera->SetAutoActivate(false);
    static ConstructorHelpers::FObjectFinder<UCameraAsset> SideScrollCamera(
        TEXT("/Game/Delerict/Cameras/CA_DSideScroll.CA_DSideScroll"));
    GameplayCamera->CameraReference.SetCameraAsset(SideScrollCamera.Object);
}

void ADPawn::BeginPlay()
{
    Super::BeginPlay();
    PlaceholderMesh->SetVisibility(!Mesh->GetSkeletalMeshAsset());

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
    Mover->OnPostFinalize.AddUniqueDynamic(this, &ThisClass::UpdateCameraLead);
}

void ADPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearInput();
    Super::EndPlay(Reason);
}

void ADPawn::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (APlayerController* Player = Cast<APlayerController>(NewController);
        Player && Player->IsLocalController() && GameplayCamera->CameraReference.IsValid())
    {
        GameplayCamera->ActivateCameraForPlayerController(Player);
    }
}

void ADPawn::UnPossessed()
{
    GameplayCamera->Deactivate();
    ClearInput();
    Super::UnPossessed();
}

FVector ADPawn::GetVelocity() const
{
    return Mover ? Mover->GetVelocity() : FVector::ZeroVector;
}

void ADPawn::ProduceInput_Implementation(int32 /*SimTimeMs*/, FMoverInputCmdContext& InputCmd)
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

void ADPawn::UpdateCameraLead(const FMoverSyncState& /*SyncState*/, const FMoverAuxStateContext& /*AuxState*/)
{
    const double MaxLead = CameraMaxLookAhead;
    const double Lead = FMath::Clamp(GetVelocity().X * CameraLookAheadTime, -MaxLead, MaxLead);
    GameplayCamera->SetWorldLocation(GetActorLocation() + FVector(Lead, 0., 0.));
}

void ADPawn::CreateInputMappings()
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

void ADPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
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

void ADPawn::ClearInput()
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

void ADPawn::Move(const FInputActionValue& Value) { HorizontalIntent = Value.Get<float>(); }
void ADPawn::StopMoving() { HorizontalIntent = 0.f; }

void ADPawn::StartJump()
{
    bJumpPressed = true;
    bJumpRequested = true;
}

void ADPawn::StopJump() { bJumpPressed = false; }
