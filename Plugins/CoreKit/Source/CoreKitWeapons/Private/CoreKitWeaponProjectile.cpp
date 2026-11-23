#include "CoreKitWeaponProjectile.h"

#include "Components/StaticMeshComponent.h"
#include "CoreKitWeapons/CoreKitWeaponDefinition.h"
#include "CoreKitWeapons/CoreKitWeaponReceiver.h"
#include "CoreKitWeapons/CoreKitWeaponsComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

ACoreKitWeaponProjectile::ACoreKitWeaponProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    SetRootComponent(Visual);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCanEverAffectNavigation(false);
    SetActorHiddenInGame(true);
}

void ACoreKitWeaponProjectile::Launch(UCoreKitWeaponsComponent& Source, UCoreKitWeaponDefinition& InDefinition,
    const FCoreKitWeaponAim& Aim, const FCoreKitWeaponImpact& Impact)
{
    check(!bLeased);
    bLeased = true;
    Weapons = &Source;
    Definition = &InDefinition;
    Snapshot = Impact;
    Direction = Aim.Direction;
    PlaneNormal = Aim.PlaneNormal;
    Remaining = FMath::Min(Definition->ProjectileLifetime, Definition->Range / Definition->ProjectileSpeed);
    Penetrations = FMath::Clamp(Definition->Penetrations, 0, 16);
    Ricochets = FMath::Clamp(Definition->Ricochets, 0, 8);
    HitActors.Reset();
    Visual->SetStaticMesh(Definition->ProjectileMesh);
    Visual->SetWorldScale3D(Definition->ProjectileVisualScale);
    SetActorLocationAndRotation(Aim.Origin, Direction.Rotation());
    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);
}

void ACoreKitWeaponProjectile::Release()
{
    bLeased = false;
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
    HitActors.Reset();
    Weapons.Reset();
    Definition = nullptr;
    Snapshot = {};
}

AActor* ACoreKitWeaponProjectile::FindRicochetTarget(const FVector& Point) const
{
    TArray<FOverlapResult> Candidates;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoreKitWeaponRicochet), false, this);
    if (IsValid(Snapshot.InstigatorActor)) Params.AddIgnoredActor(Snapshot.InstigatorActor);
    for (const auto& Actor : HitActors) if (Actor.IsValid()) Params.AddIgnoredActor(Actor.Get());
    GetWorld()->OverlapMultiByObjectType(Candidates, Point, FQuat::Identity,
        FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
        FCollisionShape::MakeSphere(Definition->RicochetRadius), Params);
    AActor* Best = nullptr;
    double BestSq = FMath::Square(Definition->RicochetRadius);
    for (const FOverlapResult& Candidate : Candidates)
    {
        AActor* Actor = Candidate.GetActor();
        if (!IsValid(Actor) || !Actor->Implements<UCoreKitWeaponReceiver>()) continue;
        const FVector Delta = Actor->GetActorLocation() - Point;
        if (Delta.SquaredLength() >= BestSq ||
            FMath::Abs(FVector::DotProduct(Delta, PlaneNormal)) > Definition->CollisionRadius * 2.f) continue;
        FHitResult Wall;
        if (GetWorld()->LineTraceSingleByChannel(Wall, Point, Point + Delta, Definition->TraceChannel, Params) &&
            Wall.GetActor() != Actor) continue;
        Best = Actor;
        BestSq = Delta.SquaredLength();
    }
    return Best;
}

void ACoreKitWeaponProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bLeased) return;
    if (!Weapons.IsValid() || !Definition || !IsValid(Snapshot.InstigatorActor))
    {
        Release();
        return;
    }
    double Distance = FMath::Min(DeltaSeconds, Remaining) * Definition->ProjectileSpeed;
    Remaining -= DeltaSeconds;
    // A bounded number of re-sweeps prevents multiple colliders from stalling a frame.
    for (int32 Step = 0; Step < 32 && Distance > UE_SMALL_NUMBER; ++Step)
    {
        const FVector Start = GetActorLocation();
        const FVector End = Start + Direction * Distance;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CoreKitWeaponProjectile), false, this);
        Params.AddIgnoredActor(Snapshot.InstigatorActor);
        for (const auto& Actor : HitActors) if (Actor.IsValid()) Params.AddIgnoredActor(Actor.Get());
        FCoreKitWeaponImpact Impact = Snapshot;
        Impact.Direction = Direction;
        if (!GetWorld()->SweepSingleByChannel(Impact.Hit, Start, End, FQuat::Identity, Definition->TraceChannel,
            FCollisionShape::MakeSphere(Definition->CollisionRadius), Params))
        {
            SetActorLocationAndRotation(End, Direction.Rotation());
            break;
        }
        SetActorLocation(Impact.Hit.Location);
        const bool bAccepted = Weapons->ReportImpact(Impact);
        if (IsActorBeingDestroyed() || !Weapons.IsValid()) return;
        if (bAccepted && IsValid(Impact.Hit.GetActor()) && Definition->BlastRadius == 0.f)
        {
            HitActors.Add(Impact.Hit.GetActor());
            if (AActor* Next = Ricochets > 0 ? FindRicochetTarget(GetActorLocation()) : nullptr)
            {
                --Ricochets;
                Direction = FVector::VectorPlaneProject(Next->GetActorLocation() - GetActorLocation(), PlaneNormal).GetSafeNormal();
                Distance -= FMath::Max(.1f, Impact.Hit.Distance);
                continue;
            }
            if (Penetrations > 0)
            {
                --Penetrations;
                Distance -= FMath::Max(.1f, Impact.Hit.Distance);
                continue;
            }
        }
        Weapons->ResolveBlast(*Definition, Impact);
        Release();
        return;
    }
    if (Remaining <= 0.f) Release();
}
