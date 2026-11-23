#include "DCompletoThrowable.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

ADCompletoThrowable::ADCompletoThrowable()
{
    ThrowableType = TEXT("CompletoItaliano");
    Collision->InitSphereRadius(18.f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Completo(
        TEXT("/Game/Delerict/Props/Completo/SM_DCompleto.SM_DCompleto"));
    Mesh->SetStaticMesh(Completo.Object);
    Mesh->SetRelativeRotation(FRotator::ZeroRotator);
    Mesh->SetRelativeScale3D(FVector::OneVector);
}

void ADCompletoThrowable::BeginPlay()
{
    Super::BeginPlay();
    Movement->OnProjectileBounce.AddUniqueDynamic(this, &ThisClass::SquashOnImpact);
}

bool ADCompletoThrowable::CanGrab() const
{
    return !bSquashed && Super::CanGrab();
}

void ADCompletoThrowable::ResetDeployment()
{
    // Leave the level's floating animation when the player picks up the food.
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ADCompletoThrowable::AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal)
{
    if (CanGrab() && !VelocityChange.IsNearlyZero())
    {
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Super::AddDriftVelocity(VelocityChange, PlaneNormal);
    }
}

void ADCompletoThrowable::SquashOnImpact(const FHitResult& Hit, const FVector& /*ImpactVelocity*/)
{
    if (bSquashed)
    {
        return;
    }

    bSquashed = true;
    Movement->StopMovementImmediately();
    SetActorEnableCollision(false);

    // Squash the mesh against the actual surface so floors, walls, and ceilings all work.
    const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
    const FVector Scale = Mesh->GetComponentScale() * FVector(1.12, 1.12, .18);
    const FQuat Rotation = FRotationMatrix::MakeFromZX(Normal, Mesh->GetForwardVector()).ToQuat();
    if (const UStaticMesh* Shape = Mesh->GetStaticMesh())
    {
        const FBox Bounds = Shape->GetBoundingBox();
        const FVector Center = Bounds.GetCenter();
        const FVector Bottom(Center.X, Center.Y, Bounds.Min.Z);
        const FVector Position = Hit.ImpactPoint + Normal * .5 - Rotation.RotateVector(Bottom * Scale);
        Mesh->SetWorldLocationAndRotation(Position, Rotation);
        Mesh->SetWorldScale3D(Scale);
    }

    if (UPrimitiveComponent* Surface = Hit.GetComponent())
    {
        AttachToComponent(Surface, FAttachmentTransformRules::KeepWorldTransform);
    }

#if DELERICT_WITH_GAMEPLAY_DEBUG
    using namespace DGameplayDebug;
    if (IsEnabled(GetWorld(), EChannel::Throws))
    {
        DrawLabel(GetWorld(), EChannel::Throws, Hit.ImpactPoint, TEXT("Completo squashed"), 1.5f);
    }
#endif
}
