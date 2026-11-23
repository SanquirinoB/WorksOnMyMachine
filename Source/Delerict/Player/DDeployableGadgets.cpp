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
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"
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

    enum class EDGadgetQuery
    {
        GrenadeBurst,
        BlackHoleBurst,
        BlackHoleField
    };

    USceneComponent* GetMovableRoot(const AActor& Actor)
    {
        USceneComponent* Root = Actor.GetRootComponent();
        return IsValid(Root) && Root->Mobility == EComponentMobility::Movable ? Root : nullptr;
    }

    bool HasGadgetSight(const UWorld& World, const FVector& Origin, const FVector& TargetPoint,
        const AActor& Target, const FCollisionQueryParams& Query)
    {
        FCollisionQueryParams Sight = Query;
        Sight.AddIgnoredActor(&Target);
        return !World.LineTraceTestByChannel(Origin, TargetPoint, ECC_Visibility, Sight);
    }

    TArray<FDGadgetTarget> FindGadgetTargets(const AActor& Source, float Radius, EDGadgetQuery Mode,
        const AActor* Ignored = nullptr)
    {
        TArray<FDGadgetTarget> Targets;
        UWorld* World = Source.GetWorld();
        if (!World || !World->IsGameWorld() || Radius <= 0.f)
        {
            return Targets;
        }

        const FVector Origin = Source.GetActorLocation();
        const double RadiusSquared = FMath::Square(static_cast<double>(Radius));
        const bool bBlackHole = Mode != EDGadgetQuery::GrenadeBurst;
        FCollisionObjectQueryParams DynamicObjects(FCollisionObjectQueryParams::AllDynamicObjects);
        if (!bBlackHole)
        {
            DynamicObjects = FCollisionObjectQueryParams();
            DynamicObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
            DynamicObjects.AddObjectTypesToQuery(ECC_Pawn);
            DynamicObjects.AddObjectTypesToQuery(ECC_PhysicsBody);
        }
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
            const bool bAuthoredTarget = Actor->ActorHasTag(GadgetTargetTag) && (!bBlackHole || GetMovableRoot(*Actor));
            const bool bProjectile = bBlackHole && GetMovableRoot(*Actor)
                && Actor->FindComponentByClass<UProjectileMovementComponent>();
            if (Throwable ? !Throwable->CanGrab()
                : !Actor->IsA<APawn>() && !Primitive->IsSimulatingPhysics() && !bAuthoredTarget && !bProjectile)
            {
                continue;
            }

            UPrimitiveComponent*& Candidate = Candidates.FindOrAdd(Actor);
            if (!Candidate || Primitive->IsSimulatingPhysics())
            {
                Candidate = Primitive;
            }
        }

        if (bBlackHole)
        {
            // Explicitly opted-in props may have query-disabled or custom collision.
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                AActor* Actor = *It;
                if (Actor == &Source || Actor == Ignored || Actor->IsActorBeingDestroyed()
                    || FVector::DistSquared(Origin, Actor->GetActorLocation()) > RadiusSquared
                    || !Actor->ActorHasTag(GadgetTargetTag) || !GetMovableRoot(*Actor) || Candidates.Contains(Actor))
                {
                    continue;
                }
                const auto* Throwable = Cast<ADThrowable>(Actor);
                if (Throwable && !Throwable->CanGrab())
                {
                    continue;
                }
                if (UPrimitiveComponent* Primitive = Actor->FindComponentByClass<UPrimitiveComponent>())
                {
                    Candidates.Add(Actor, Primitive);
                }
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

            // The field reaches through walls. Movement sweeps and the core's sight check still respect them.
            if (Mode == EDGadgetQuery::BlackHoleField || HasGadgetSight(*World, Origin, Center, *Candidate.Key, Query))
            {
                Targets.Add({ Candidate.Key, Candidate.Value, Offset });
            }
        }
        return Targets;
    }

    void AddPropVelocity(AActor& Actor, const FVector& DeltaVelocity)
    {
        USceneComponent* Root = GetMovableRoot(Actor);
        if (!Root)
        {
            return;
        }

        auto* Movement = Actor.FindComponentByClass<UProjectileMovementComponent>();
        if (!Movement)
        {
            // Give an authored loose prop inertial motion that persists after the field shuts down.
            const FVector PreviousVelocity = Actor.GetVelocity();
            Movement = NewObject<UProjectileMovementComponent>(&Actor);
            Movement->bAutoActivate = false;
            Movement->bInitialVelocityInLocalSpace = false;
            Movement->ProjectileGravityScale = 0.f;
            Movement->bShouldBounce = true;
            Movement->Bounciness = .35f;
            Movement->Velocity = PreviousVelocity;
            Actor.AddInstanceComponent(Movement);
            Movement->SetUpdatedComponent(Root);
            Movement->RegisterComponent();
        }
        else if (!Movement->UpdatedComponent)
        {
            Movement->SetUpdatedComponent(Root);
        }

        Movement->Velocity += Movement->ConstrainDirectionToPlane(DeltaVelocity);
        Movement->UpdateComponentVelocity();
        Movement->Activate();
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
        else if (auto* Character = Cast<ACharacter>(Target.Actor))
        {
            Character->GetCharacterMovement()->AddImpulse(DeltaVelocity, true);
        }
        else if (auto* OtherPawn = Cast<APawn>(Target.Actor); OtherPawn && OtherPawn->GetMovementComponent())
        {
            UPawnMovementComponent* Movement = OtherPawn->GetMovementComponent();
            Movement->Velocity += Movement->ConstrainDirectionToPlane(DeltaVelocity);
        }
        else
        {
            AddPropVelocity(*Target.Actor, DeltaVelocity);
        }
    }

    void Burst(ADThrowable& Source, const ADPawn* Deployer, float Damage, float Radius,
        float PushSpeed, const FVector& PlaneNormal, const FLinearColor& Color, EDGadgetQuery Mode,
        const AActor* Ignored = nullptr)
    {
        UWorld* World = Source.GetWorld();
        if (!World || !World->IsGameWorld())
        {
            return;
        }
        const FVector Origin = Source.GetActorLocation();
        AController* Instigator = Deployer ? Deployer->GetController() : nullptr;
        for (const FDGadgetTarget& Target : FindGadgetTargets(Source, Radius, Mode, Ignored))
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
            const bool bRupture = Mode == EDGadgetQuery::BlackHoleBurst;
            Light->SetIntensity(bRupture ? 32000.f : 8500.f);
            Light->SetLightColor(Color);
            Light->SetAttenuationRadius(FMath::Max(150.f, Radius));
            Flash->SetLifeSpan(bRupture ? .32f : .18f);
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
        FVector ClosestPoint;
        const float Distance = Target.Primitive->GetClosestPointOnCollision(Origin, ClosestPoint);
        return Distance >= 0.f && Distance <= Radius;
    }

    bool ConsumeActor(AActor& Actor)
    {
        if (auto* Pawn = Cast<APawn>(&Actor))
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

    ADPawn* Pawn = GetDeployingPawn();
    if (!Pawn)
    {
        ResetDeployment();
        return;
    }

    if (bReelingIn && UpdateRecall(*Pawn, DeltaSeconds))
    {
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
    bReelingIn = false;
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
    if (bAttachedToAnchor || bReelingIn || !IsValid(Actor) || Actor == GetDeployingPawn() || !IsValid(Component))
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
        Movement->Deactivate();
        SetActorEnableCollision(false);
    }
}

void ADGrapplingHook::OnRemoteActivationRequested(ADPawn& Pawn)
{
    if (!Anchor.IsValid())
    {
        if (RecallSpeed > 0.f && !bReelingIn)
        {
            ClearAnchor();
            Movement->StopMovementImmediately();
            Movement->Deactivate();
            bReelingIn = true;
        }
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

bool ADGrapplingHook::UpdateRecall(ADPawn& Pawn, float DeltaSeconds)
{
    const FVector HookLocation = GetActorLocation();
    const FVector TowardPawn = Pawn.GetActorLocation() - HookLocation;
    const double Distance = TowardPawn.Size();
    const double CatchDistance = Pawn.GetSimpleCollisionRadius() + GetCollisionRadius();
    if (Distance <= CatchDistance + UE_KINDA_SMALL_NUMBER)
    {
        bReelingIn = false;
        Movement->Velocity = Pawn.GetVelocity();
        if (Pawn.TryGrabThrowable(*this))
        {
            return true;
        }
        Movement->StopMovementImmediately();
        return false;
    }

    const FVector Direction = TowardPawn / Distance;
    const double Step = FMath::Min(static_cast<double>(RecallSpeed) * DeltaSeconds, Distance - CatchDistance);
    FHitResult Hit;
    SetActorLocationAndRotation(HookLocation + Direction * Step, Direction.Rotation(), true, &Hit);
    if (Hit.bBlockingHit)
    {
        bReelingIn = false;
    }
    return false;
}

void ADGrapplingHook::AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal)
{
    if (!bAttachedToAnchor && !bReelingIn)
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
            FLinearColor(1.f, .35f, .04f), EDGadgetQuery::GrenadeBurst);
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

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Emissive(
        TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
    FieldCore = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldCore"));
    FieldOrbit = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldOrbit"));
    FieldOrbitOpposite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FieldOrbitOpposite"));
    for (UStaticMeshComponent* Visual : {FieldCore.Get(), FieldOrbit.Get(), FieldOrbitOpposite.Get()})
    {
        Visual->SetupAttachment(Collision);
        Visual->SetStaticMesh(Sphere.Object);
        Visual->SetMaterial(0, Emissive.Object);
        Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Visual->SetCastShadow(false);
        Visual->SetCanEverAffectNavigation(false);
        Visual->SetVisibility(false);
    }
    FieldOrbit->SetRelativeScale3D(FVector(.12));
    FieldOrbitOpposite->SetRelativeScale3D(FVector(.08));

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

void ADPortableBlackHoleDevice::BeginPlay()
{
    Super::BeginPlay();
    FieldCore->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(.012, .002, .025));
    FieldOrbit->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(2.5, .26, 6.));
    FieldOrbitOpposite->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(.9, .14, 4.));
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
    FieldCore->SetVisibility(bActive);
    FieldOrbit->SetVisibility(bActive);
    FieldOrbitOpposite->SetVisibility(bActive);
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

    const float VisibleCoreRadius = FMath::Max(20.f, CoreRadius);
    FieldCore->SetRelativeScale3D(FVector(VisibleCoreRadius * .02));
    const double Phase = GetWorld()->GetTimeSeconds() * 4.;
    const FVector Orbit = FVector(FMath::Cos(Phase), 0., FMath::Sin(Phase)) * (VisibleCoreRadius + 12.f);
    FieldOrbit->SetRelativeLocation(Orbit);
    FieldOrbitOpposite->SetRelativeLocation(-Orbit);

    const TArray<FDGadgetTarget> Targets = FindGadgetTargets(*this, AttractionRadius, EDGadgetQuery::BlackHoleField);
    const FVector Origin = GetActorLocation();
    const FCollisionQueryParams CoreSight(SCENE_QUERY_STAT(DelerictBlackHoleCore), false, this);
    const FVector PlaneNormal = Movement->GetPlaneConstraintNormal();
    for (const FDGadgetTarget& Target : Targets)
    {
        if (CoreRadius > 0.f && ReachedCore(Target, Origin, CoreRadius)
            && HasGadgetSight(*GetWorld(), Origin, Origin + Target.Offset, *Target.Actor, CoreSight)
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
        Movement->GetPlaneConstraintNormal(), FLinearColor(.65f, .25f, 1.f), EDGadgetQuery::BlackHoleBurst, &ConsumedActor);
    Destroy();
}
