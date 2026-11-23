#include "DThrowable.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DPawn.h"
#include "DThrowSwitch.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

ADThrowable::ADThrowable()
{
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(15.f);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Collision->SetGenerateOverlapEvents(false);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Disc(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Mesh->SetStaticMesh(Disc.Object);
    Mesh->SetRelativeRotation(FRotator(0., 0., 65.));
    Mesh->SetRelativeScale3D(FVector(.28, .28, .07));

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Collision);
    Movement->bAutoActivate = false;
    Movement->Velocity = FVector::ZeroVector;
    Movement->ProjectileGravityScale = 0.f;
    Movement->bShouldBounce = true;
    Movement->Bounciness = .35f;
    Movement->Friction = .15f;
    Movement->BounceVelocityStopSimulatingThreshold = 10.f;
}

void ADThrowable::BeginPlay()
{
    Super::BeginPlay();
    Movement->OnProjectileBounce.AddUniqueDynamic(this, &ThisClass::OnBounce);
}

FVector ADThrowable::GetVelocity() const
{
    return Movement->Velocity;
}

float ADThrowable::GetCollisionRadius() const
{
    return Collision->GetScaledSphereRadius();
}

bool ADThrowable::CanGrab() const
{
    return !Holder.IsValid() && !IsHidden() && !IsActorBeingDestroyed();
}

bool ADThrowable::IsHeldBy(const ADPawn& Pawn) const
{
    return Holder.Get() == &Pawn;
}

bool ADThrowable::TryHold(ADPawn& Pawn)
{
    if (!CanGrab() || (IsReusableAfterPickup() && LastThrower.IsValid() && LastThrower.Get() != &Pawn))
    {
        return false;
    }
    ResetDeployment();
    Holder = &Pawn;
    LastThrower.Reset();
    Movement->Deactivate();
    Movement->Velocity = FVector::ZeroVector;
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    return true;
}

bool ADThrowable::TryReturnToOwner()
{
    ADPawn* Pawn = GetDeployingPawn();
    return IsReusableAfterPickup() && Pawn && Pawn->TryStowReusableThrowable(*this);
}

void ADThrowable::Launch(const FVector& Location, const FVector& Velocity, const FVector& PlaneNormal, ADPawn* Thrower)
{
    ResetDeployment();
    Holder.Reset();
    LastThrower = Thrower;
    SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    Movement->SetUpdatedComponent(Collision);
    Movement->SetPlaneConstraintOrigin(Location);
    Movement->SetPlaneConstraintNormal(PlaneNormal);
    Movement->SetPlaneConstraintEnabled(!PlaneNormal.IsNearlyZero());
    Movement->Velocity = Movement->ConstrainDirectionToPlane(Velocity);
    Movement->UpdateComponentVelocity();
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
    Movement->Activate(true);
    OnDeployed(Thrower);
}

ADPawn* ADThrowable::GetDeployingPawn() const
{
    return LastThrower.Get();
}

bool ADThrowable::TryActivate(ADPawn& Pawn)
{
    if (!bRemoteActivatable || LastThrower.Get() != &Pawn || !CanGrab())
    {
        return false;
    }
    OnRemoteActivated.Broadcast();
    if (IsValid(this) && LastThrower.Get() == &Pawn)
    {
        OnRemoteActivationRequested(Pawn);
    }
    return true;
}

void ADThrowable::AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal)
{
    if (!CanGrab() || VelocityChange.IsNearlyZero())
    {
        return;
    }

    if (!Movement->bConstrainToPlane)
    {
        Movement->SetPlaneConstraintOrigin(GetActorLocation());
        Movement->SetPlaneConstraintNormal(PlaneNormal);
        Movement->SetPlaneConstraintEnabled(!PlaneNormal.IsNearlyZero());
    }
    if (Movement->UpdatedComponent.Get() != Collision.Get())
    {
        Movement->SetUpdatedComponent(Collision);
    }
    Movement->Velocity += Movement->ConstrainDirectionToPlane(VelocityChange);
    Movement->UpdateComponentVelocity();
    Movement->Activate();
}

void ADThrowable::OnBounce(const FHitResult& Hit, const FVector& /*ImpactVelocity*/)
{
    if (auto* Switch = Cast<ADThrowSwitch>(Hit.GetActor()))
    {
        Switch->Activate();
#if DELERICT_WITH_GAMEPLAY_DEBUG
        using namespace DGameplayDebug;
        if (IsEnabled(GetWorld(), EChannel::Throws))
        {
            DrawLabel(GetWorld(), EChannel::Throws, Hit.ImpactPoint,
                FString::Printf(TEXT("%s hit %s | switch active"), *ThrowableType.ToString(), *Switch->GetName()), 1.5f);
        }
#endif
    }
    if (IsValid(this) && LastThrower.IsValid())
    {
        OnDeployedImpact(Hit);
    }
}
