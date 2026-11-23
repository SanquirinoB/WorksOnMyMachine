#include "DPawn.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/CameraAsset.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "DLocomotionAnimInstance.h"
#include "DMoverComponent.h"
#include "DPlayerTuning.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "MoveLibrary/ConstrainedMoveUtils.h"
#include "MoverDataModelTypes.h"
#include "UObject/ConstructorHelpers.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

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

void ADPawn::PreInitializeComponents()
{
    Super::PreInitializeComponents();
    const UDPlayerTuning& Tuning = GetTuning();
    Capsule->SetCapsuleSize(Tuning.CapsuleRadius, Tuning.CapsuleHalfHeight);
    Capsule->SetCollisionProfileName(Tuning.CapsuleCollisionProfile.Name);
    SetActorRotation(Tuning.LockedRotation, ETeleportType::TeleportPhysics);
    Mover->SetGravityOverride(true, Tuning.GravityAcceleration);

    FPlanarConstraint Constraint;
    Constraint.bConstrainToPlane = true;
    Constraint.PlaneConstraintNormal = Tuning.MovementPlaneNormal.GetSafeNormal();
    if (Constraint.PlaneConstraintNormal.IsNearlyZero())
    {
        Constraint.PlaneConstraintNormal = FVector::RightVector;
    }
    Constraint.PlaneConstraintOrigin = GetActorLocation();
    Mover->SetPlanarConstraint(Constraint);

    if (!SetAimDirection(Tuning.InitialAimDirection))
    {
        const FVector Normal = Constraint.PlaneConstraintNormal;
        SetAimDirection(FMath::Abs(Normal.X) < .9 ? FVector::ForwardVector : FVector::UpVector);
    }
}

void ADPawn::BeginPlay()
{
    Super::BeginPlay();
    PlaceholderMesh->SetVisibility(!Mesh->GetSkeletalMeshAsset());
    Mover->OnPostFinalize.AddUniqueDynamic(this, &ThisClass::UpdateCameraLead);
}

void ADPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearInput();
    RestoreAimCursor();
    Super::EndPlay(Reason);
}

void ADPawn::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (APlayerController* Player = Cast<APlayerController>(NewController); Player && Player->IsLocalController())
    {
        if (GameplayCamera->CameraReference.IsValid())
        {
            GameplayCamera->ActivateCameraForPlayerController(Player);
        }
        SetUpAimCursor(*Player);
    }
}

void ADPawn::UnPossessed()
{
    GameplayCamera->Deactivate();
    ClearInput();
    RestoreAimCursor();
    Super::UnPossessed();
}

FVector ADPawn::GetVelocity() const
{
    return Mover ? Mover->GetVelocity() : FVector::ZeroVector;
}

const UDPlayerTuning* ADPawn::GetPlayerTuning() const
{
    return PlayerTuning;
}

const UDPlayerTuning& ADPawn::GetTuning() const
{
    return PlayerTuning ? *PlayerTuning : *GetDefault<UDPlayerTuning>();
}

float ADPawn::GetEffectiveMass() const
{
    const float Mass = GetTuning().PlayerMass;
    return FMath::IsFinite(Mass) ? FMath::Max(Mass, .01f) : GetDefault<UDPlayerTuning>()->PlayerMass;
}

void ADPawn::QueueImpulse(const FVector& WorldImpulse)
{
    check(IsInGameThread());
    if (WorldImpulse.ContainsNaN())
    {
        return;
    }

    // Snapshot the mass here so later inventory events cannot change the effect of an earlier impulse.
    PendingImpulses.Add({ WorldImpulse, GetEffectiveMass() });
}

void ADPawn::ProduceInput_Implementation(int32 /*SimTimeMs*/, FMoverInputCmdContext& InputCmd)
{
    check(IsInGameThread());
    UpdateAim();

    FCharacterDefaultInputs& Inputs = InputCmd.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
    Inputs.SetMoveInput(EMoveInputType::DirectionalIntent, FVector::ZeroVector);
    Inputs.OrientationIntent = FVector::ZeroVector;
    Inputs.ControlRotation = FRotator::ZeroRotator;
    Inputs.bIsJumpPressed = false;
    Inputs.bIsJumpJustPressed = false;

#if DELERICT_WITH_GAMEPLAY_DEBUG
    const bool bDebugFlight = DGameplayDebug::IsEnabled(GetWorld(), DGameplayDebug::EChannel::Flight);
    DebugImpulse = FVector::ZeroVector;
    if (DGameplayDebug::IsEnabled(GetWorld(), DGameplayDebug::EChannel::Collision))
    {
        DebugSweepStart = GetActorLocation();
    }
#endif

    FVector VelocityChange = FVector::ZeroVector;
    const FPlanarConstraint& Plane = Mover->GetPlanarConstraint();
    for (const FQueuedImpulse& Impulse : PendingImpulses)
    {
        const FVector InPlane = UPlanarConstraintUtils::ConstrainDirectionToPlane(Plane, Impulse.WorldImpulse);
        VelocityChange += InPlane / Impulse.Mass;
#if DELERICT_WITH_GAMEPLAY_DEBUG
        if (bDebugFlight)
        {
            DebugImpulse += InPlane;
        }
#endif
    }
    PendingImpulses.Reset();
#if DELERICT_WITH_GAMEPLAY_DEBUG
    if (bDebugFlight)
    {
        DebugVelocityChange = VelocityChange;
    }
#endif

    if (!VelocityChange.IsNearlyZero())
    {
        auto Effect = MakeShared<FApplyVelocityEffect>();
        Effect->VelocityToApply = VelocityChange;
        Effect->bAdditiveVelocity = true;
        Mover->QueueInstantMovementEffect(Effect);
    }
}

void ADPawn::UpdateCameraLead(const FMoverSyncState& /*SyncState*/, const FMoverAuxStateContext& /*AuxState*/)
{
    const FVector Location = GetActorLocation();
    const FVector Velocity = GetVelocity();
    const UDPlayerTuning& Tuning = GetTuning();
    const double MaxLead = FMath::Max(0.f, Tuning.CameraMaxLookAhead);
    const double Lead = FMath::Clamp(Velocity.X * Tuning.CameraLookAheadTime, -MaxLead, MaxLead);
    GameplayCamera->SetWorldLocation(Location + FVector(Lead, 0., 0.));

#if DELERICT_WITH_GAMEPLAY_DEBUG
    using namespace DGameplayDebug;
    UWorld* World = GetWorld();
    if (IsEnabled(World, EChannel::Flight))
    {
        const FVector Gravity = Mover->GetGravityAcceleration();
        DrawArrow(World, EChannel::Flight, Location, Location + (Velocity * .25).GetClampedToMaxSize(240.));
        DrawLabel(World, EChannel::Flight, Location + FVector(0., 0., 150.), FString::Printf(
            TEXT("%s | gravity %s cm/s^2 | velocity %s cm/s (%.1f) | mass %.1f kg | impulse %s kg*cm/s | delta-v %s cm/s | camera lead %.1f cm"),
            *Mover->GetMovementModeName().ToString(), *Gravity.ToCompactString(), *Velocity.ToCompactString(),
            Velocity.Size(), GetEffectiveMass(), *DebugImpulse.ToCompactString(), *DebugVelocityChange.ToCompactString(), Lead));
    }

    const TOptional<FHitResult> Impact = Mover->TakeFlightImpact();
    if (IsEnabled(World, EChannel::Collision))
    {
        DrawCapsule(World, EChannel::Collision, Location, Capsule->GetScaledCapsuleHalfHeight(),
            Capsule->GetScaledCapsuleRadius(), Capsule->GetComponentRotation());
        if (Impact)
        {
            const FVector Slide = Location - Impact->Location;
            DrawLine(World, EChannel::Collision, Impact->TraceStart, Impact->TraceEnd);
            DrawArrow(World, EChannel::Collision, Impact->ImpactPoint, Impact->ImpactPoint + Impact->ImpactNormal * 60.);
            DrawArrow(World, EChannel::Collision, Impact->Location, Location);
            DrawLabel(World, EChannel::Collision, Impact->ImpactPoint, FString::Printf(
                TEXT("Impact %s | slide %s cm"), *GetNameSafe(Impact->GetActor()), *Slide.ToCompactString()), .1f);
        }
        else
        {
            DrawLine(World, EChannel::Collision, DebugSweepStart, Location);
        }
    }
#endif
}
