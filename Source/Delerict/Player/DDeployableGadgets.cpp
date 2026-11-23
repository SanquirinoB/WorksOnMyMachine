#include "DDeployableGadgets.h"

#include "../LevelDesign/DRoomProgress.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DPawn.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#include "DrawDebugHelpers.h"
#endif

namespace
{
    const FName GrappleAnchorTag(TEXT("Delerict.GrappleAnchor"));
    const FName GadgetTargetTag(TEXT("Delerict.GadgetTarget"));

    struct FDGadgetTarget
    {
        AActor* Actor;
        UPrimitiveComponent* Primitive;
        FVector Offset;
    };

    TArray<FDGadgetTarget> FindGadgetTargets(const AActor& Source, float Radius, const AActor* Ignored = nullptr)
    {
        TArray<FDGadgetTarget> Targets;
        UWorld* World = Source.GetWorld();
        if (!World || !World->IsGameWorld() || Radius <= 0.f)
        {
            return Targets;
        }

        const FVector Origin = Source.GetActorLocation();
        const double RadiusSquared = FMath::Square(static_cast<double>(Radius));
        FCollisionObjectQueryParams DynamicObjects;
        DynamicObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
        DynamicObjects.AddObjectTypesToQuery(ECC_Pawn);
        DynamicObjects.AddObjectTypesToQuery(ECC_PhysicsBody);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DelerictGadgetTargets), false, &Source);
        if (Ignored)
        {
            Query.AddIgnoredActor(Ignored);
        }
        TArray<FOverlapResult> Overlaps;
        World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, DynamicObjects,
            FCollisionShape::MakeSphere(Radius), Query);

        TMap<AActor*, UPrimitiveComponent*> Candidates;
        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* Actor = Overlap.GetActor();
            UPrimitiveComponent* Primitive = Overlap.GetComponent();
            if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || !IsValid(Primitive))
            {
                continue;
            }

            const ADThrowable* Throwable = Cast<ADThrowable>(Actor);
            if (Throwable ? !Throwable->CanGrab()
                : !Actor->IsA<APawn>() && !Primitive->IsSimulatingPhysics() && !Actor->ActorHasTag(GadgetTargetTag))
            {
                continue;
            }

            UPrimitiveComponent*& Candidate = Candidates.FindOrAdd(Actor);
            if (!Candidate || Primitive->IsSimulatingPhysics())
            {
                Candidate = Primitive;
            }
        }

        for (const auto& Candidate : Candidates)
        {
            const FVector Center = Candidate.Value->IsSimulatingPhysics()
                ? Candidate.Value->GetCenterOfMass() : Candidate.Key->GetActorLocation();
            const FVector Offset = Center - Origin;
            if (Offset.SizeSquared() > RadiusSquared)
            {
                continue;
            }

            FCollisionQueryParams Sight = Query;
            Sight.AddIgnoredActor(Candidate.Key);
            if (!World->LineTraceTestByChannel(Origin, Center, ECC_Visibility, Sight))
            {
                Targets.Add({ Candidate.Key, Candidate.Value, Offset });
            }
        }
        return Targets;
    }

    void AddGadgetVelocity(const FDGadgetTarget& Target, const FVector& DeltaVelocity, const FVector& PlaneNormal)
    {
        if (DeltaVelocity.IsNearlyZero())
        {
            return;
        }
        if (auto* Pawn = Cast<ADPawn>(Target.Actor))
        {
            Pawn->QueueImpulse(DeltaVelocity * Pawn->GetEffectiveMass());
        }
        else if (auto* Throwable = Cast<ADThrowable>(Target.Actor))
        {
            Throwable->AddDriftVelocity(DeltaVelocity, PlaneNormal);
        }
        else if (Target.Primitive->IsSimulatingPhysics())
        {
            Target.Primitive->AddImpulse(DeltaVelocity, NAME_None, true);
        }
        else if (auto* OtherPawn = Cast<APawn>(Target.Actor))
        {
            if (UPawnMovementComponent* Movement = OtherPawn->GetMovementComponent())
            {
                Movement->Velocity += DeltaVelocity;
            }
        }
    }

    void Burst(ADThrowable& Source, const ADPawn* Deployer, float Damage, float Radius,
        float PushSpeed, const FVector& PlaneNormal, const FLinearColor& Color, const AActor* Ignored = nullptr)
    {
        UWorld* World = Source.GetWorld();
        if (!World || !World->IsGameWorld())
        {
            return;
        }
        const FVector Origin = Source.GetActorLocation();
        AController* Instigator = Deployer ? Deployer->GetController() : nullptr;
        for (const FDGadgetTarget& Target : FindGadgetTargets(Source, Radius, Ignored))
        {
            if (!IsValid(Target.Actor) || Target.Actor->IsActorBeingDestroyed() || !IsValid(Target.Primitive))
            {
                continue;
            }

            const float Falloff = 1.f - static_cast<float>(Target.Offset.Size()) / Radius;
            const FVector Direction = Target.Offset.GetSafeNormal();
            AddGadgetVelocity(Target, Direction * (PushSpeed * Falloff), PlaneNormal);
            if (Damage > 0.f && Target.Actor->CanBeDamaged())
            {
                FRadialDamageEvent Event;
                Event.DamageTypeClass = UDamageType::StaticClass();
                Event.Origin = Origin;
                Event.Params = FRadialDamageParams(Damage, Radius);
                Event.ComponentHits.Emplace(Target.Actor, Target.Primitive, Origin + Target.Offset, -Direction);
                Target.Actor->TakeDamage(Damage, Event, Instigator, &Source);
            }
        }

        if (auto* Flash = World->SpawnActor<APointLight>(Origin, FRotator::ZeroRotator))
        {
            UPointLightComponent* Light = Flash->PointLightComponent;
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetCastShadows(false);
            Light->SetIntensity(8500.f);
            Light->SetLightColor(Color);
            Light->SetAttenuationRadius(FMath::Max(150.f, Radius));
            Flash->SetLifeSpan(.18f);
        }
#if DELERICT_WITH_GAMEPLAY_DEBUG
        if (DGameplayDebug::IsEnabled(World, DGameplayDebug::EChannel::Throws) && Radius > 0.f)
        {
            DrawDebugSphere(World, Origin, Radius, 24, Color.ToFColor(true), false, .8f, 0, 2.f);
        }
#endif
    }

    bool ReachedCore(const FDGadgetTarget& Target, const FVector& Origin, float Radius)
    {
        if (Target.Offset.SizeSquared() <= FMath::Square(static_cast<double>(Radius)))
        {
            return true;
        }
        if (Target.Primitive->IsSimulatingPhysics())
        {
            FVector ClosestPoint;
            const float Distance = Target.Primitive->GetClosestPointOnCollision(Origin, ClosestPoint);
            return Distance >= 0.f && Distance <= Radius;
        }
        return false;
    }

    bool ConsumeActor(AActor& Actor)
    {
        if (auto* Pawn = Cast<ADPawn>(&Actor))
        {
            if (auto* Player = Pawn->GetController<APlayerController>())
            {
                if (auto* Progress = Pawn->GetWorld()->GetSubsystem<UDRoomProgress>())
                {
                    Progress->RequestRespawn(*Player, *Pawn);
                    return true;
                }
            }
        }
        return Actor.Destroy();
    }
}

ADGrapplingHook::ADGrapplingHook()
{
    ThrowableType = TEXT("GrapplingHook");
    bRemoteActivatable = true;
    Collision->InitSphereRadius(12.f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Mesh->SetStaticMesh(Cone.Object);
    Mesh->SetRelativeRotation(FRotator(90., 0., 0.));
    Mesh->SetRelativeScale3D(FVector(.17, .17, .25));
    Movement->bRotationFollowsVelocity = true;

    Tether = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tether"));
    Tether->SetupAttachment(Collision);
    Tether->SetAbsolute(true, true, true);
    Tether->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Tether->SetCastShadow(false);
    Tether->SetCanEverAffectNavigation(false);
    Tether->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Tether->SetStaticMesh(Cube.Object);

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void ADGrapplingHook::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bAttachedToAnchor && !Anchor.IsValid())
    {
        ClearAnchor();
    }

    const ADPawn* Pawn = GetDeployingPawn();
    if (!Pawn)
    {
        ResetDeployment();
        return;
    }

    const FVector Start = Pawn->GetActorLocation();
    const FVector End = GetActorLocation();
    const FVector Offset = End - Start;
    Tether->SetWorldLocationAndRotation((Start + End) * .5, Offset.Rotation());
    Tether->SetWorldScale3D(FVector(Offset.Size() * .01, .022, .022));
    Tether->SetVisibility(true);
}

void ADGrapplingHook::ClearAnchor()
{
    if (bAttachedToAnchor)
    {
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Anchor.Reset();
        bAttachedToAnchor = false;
        SetActorEnableCollision(true);
    }
}

void ADGrapplingHook::ResetDeployment()
{
    ClearAnchor();
    if (ADPawn* Pawn = GetDeployingPawn())
    {
        Collision->IgnoreActorWhenMoving(Pawn, false);
    }
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Tether->SetVisibility(false);
    SetActorTickEnabled(false);
}

void ADGrapplingHook::OnDeployed(ADPawn* Pawn)
{
    if (Pawn)
    {
        Collision->IgnoreActorWhenMoving(Pawn, true);
        Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        SetActorTickEnabled(true);
    }
}

void ADGrapplingHook::OnDeployedImpact(const FHitResult& Hit)
{
    AActor* Actor = Hit.GetActor();
    UPrimitiveComponent* Component = Hit.GetComponent();
    if (bAttachedToAnchor || !IsValid(Actor) || Actor == GetDeployingPawn() || !IsValid(Component))
    {
        return;
    }
    if (Component->Mobility == EComponentMobility::Movable
        && !Actor->IsA<APawn>() && !Actor->ActorHasTag(GrappleAnchorTag))
    {
        return;
    }

    if (AttachToComponent(Component, FAttachmentTransformRules::KeepWorldTransform))
    {
        Anchor = Component;
        bAttachedToAnchor = true;
        Movement->StopMovementImmediately();
        SetActorEnableCollision(false);
    }
}

void ADGrapplingHook::OnRemoteActivationRequested(ADPawn& Pawn)
{
    if (!Anchor.IsValid())
    {
        Pawn.TryGrabThrowable(*this);
        return;
    }

    const FVector TowardHook = Movement->ConstrainDirectionToPlane(GetActorLocation() - Pawn.GetActorLocation());
    if (PullSpeed <= 0.f || TowardHook.IsNearlyZero())
    {
        return;
    }
    const double Speed = FMath::Min(static_cast<double>(PullSpeed), TowardHook.Size() * 3.);
    const FVector DeltaVelocity = TowardHook.GetSafeNormal() * Speed - Pawn.GetVelocity();
    Pawn.QueueImpulse(DeltaVelocity * Pawn.GetEffectiveMass());
}

void ADGrapplingHook::AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal)
{
    if (!bAttachedToAnchor)
    {
        Super::AddDriftVelocity(VelocityChange, PlaneNormal);
    }
}

ADRemoteGrenade::ADRemoteGrenade()
{
    ThrowableType = TEXT("RemoteGrenade");
    bRemoteActivatable = true;
    Collision->InitSphereRadius(12.f);
    Mesh->SetRelativeRotation(FRotator::ZeroRotator);
    Mesh->SetRelativeScale3D(FVector(.16, .16, .20));
}

void ADRemoteGrenade::OnRemoteActivationRequested(ADPawn& Pawn)
{
    if (!bDetonated)
    {
        bDetonated = true;
        SetActorEnableCollision(false);
        Burst(*this, &Pawn, BurstDamage, BurstRadius, 0.f, Movement->GetPlaneConstraintNormal(),
            FLinearColor(1.f, .35f, .04f));
        Destroy();
    }
}

ADPortableBlackHoleDevice::ADPortableBlackHoleDevice()
{
    ThrowableType = TEXT("PortableBlackHoleDevice");
    bRemoteActivatable = true;
    Collision->InitSphereRadius(17.f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Mesh->SetStaticMesh(Sphere.Object);
    Mesh->SetRelativeRotation(FRotator::ZeroRotator);
    Mesh->SetRelativeScale3D(FVector(.30));

    FieldLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FieldLight"));
    FieldLight->SetupAttachment(Collision);
    FieldLight->SetCastShadows(false);
    FieldLight->SetLightColor(FLinearColor(.45f, .16f, 1.f));
    FieldLight->SetIntensity(3500.f);
    FieldLight->SetAttenuationRadius(360.f);
    FieldLight->SetVisibility(false);

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void ADPortableBlackHoleDevice::ResetDeployment()
{
    SetFieldActive(false);
}

void ADPortableBlackHoleDevice::OnRemoteActivationRequested(ADPawn& /*Pawn*/)
{
    SetFieldActive(!bFieldActive);
}

void ADPortableBlackHoleDevice::SetFieldActive(bool bActive)
{
    bFieldActive = bActive;
    FieldLight->SetVisibility(bActive);
    SetActorTickEnabled(bActive);
}

void ADPortableBlackHoleDevice::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GetDeployingPawn())
    {
        SetFieldActive(false);
        return;
    }

    const TArray<FDGadgetTarget> Targets = FindGadgetTargets(*this, AttractionRadius);
    const FVector PlaneNormal = Movement->GetPlaneConstraintNormal();
    for (const FDGadgetTarget& Target : Targets)
    {
        if (CoreRadius > 0.f && ReachedCore(Target, GetActorLocation(), CoreRadius)
            && ConsumeActor(*Target.Actor))
        {
            Rupture(*Target.Actor);
            return;
        }
    }

    const float VelocityStep = FMath::Max(0.f, AttractionAcceleration) * DeltaSeconds;
    for (const FDGadgetTarget& Target : Targets)
    {
        const double Falloff = 1. - Target.Offset.Size() / AttractionRadius;
        AddGadgetVelocity(Target, -Target.Offset.GetSafeNormal() * (VelocityStep * Falloff), PlaneNormal);
    }

#if DELERICT_WITH_GAMEPLAY_DEBUG
    UWorld* World = GetWorld();
    if (DGameplayDebug::IsEnabled(World, DGameplayDebug::EChannel::Throws) && AttractionRadius > 0.f)
    {
        DrawDebugSphere(World, GetActorLocation(), AttractionRadius, 24, FColor::Purple);
    }
#endif
}

void ADPortableBlackHoleDevice::Rupture(AActor& ConsumedActor)
{
    SetFieldActive(false);
    SetActorEnableCollision(false);
    Burst(*this, GetDeployingPawn(), RuptureDamage, RuptureRadius, RupturePushSpeed,
        Movement->GetPlaneConstraintNormal(), FLinearColor(.65f, .25f, 1.f), &ConsumedActor);
    Destroy();
}
