#include "DPawn.h"

#include "Algo/Count.h"
#include "Components/CapsuleComponent.h"
#include "DMoverComponent.h"
#include "DPlayerTuning.h"
#include "DThrowable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MoveLibrary/ConstrainedMoveUtils.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

int32 ADPawn::GetPickedUpThrowableCount() const
{
    return Algo::CountIf(GrabbedThrowables, [](const ADThrowable* Item) { return IsValid(Item); });
}

int32 ADPawn::GetThrowableCountByType(FName ThrowableType) const
{
    if (ThrowableType.IsNone())
    {
        return 0;
    }

    const int32 PickedUpCount = Algo::CountIf(GrabbedThrowables, [ThrowableType](const ADThrowable* Item)
    {
        return IsValid(Item) && Item->GetThrowableType() == ThrowableType;
    });
    return PickedUpCount + (ThrowableType == GetTuning().StartingPlateType ? PlateCount : 0);
}

const ADThrowable* ADPawn::GetGrabbedThrowable() const
{
    const auto* Item = GrabbedThrowables.FindByPredicate([](const ADThrowable* Candidate) { return IsValid(Candidate); });
    return Item ? Item->Get() : nullptr;
}

float ADPawn::GetThrowableMass(FName Type) const
{
    const float* Mass = GetTuning().ThrowableMassByType.Find(Type);
    return Mass && FMath::IsFinite(*Mass) && *Mass > 0.f ? *Mass : 0.f;
}

float ADPawn::GetEffectiveMass() const
{
    const float TunedMass = GetTuning().PlayerMass;
    const float BodyMass = FMath::IsFinite(TunedMass) ? FMath::Max(TunedMass, .01f) : GetDefault<UDPlayerTuning>()->PlayerMass;
    float Mass = BodyMass + PlateCount * GetThrowableMass(GetTuning().StartingPlateType);
    for (const ADThrowable* Item : GrabbedThrowables)
    {
        if (IsValid(Item))
        {
            Mass += GetThrowableMass(Item->GetThrowableType());
        }
    }
    return Mass;
}

FName ADPawn::GetSelectedThrowableType() const
{
    if (!SelectedThrowableType.IsNone())
    {
        return SelectedThrowableType;
    }
    if (PlateCount > 0)
    {
        return GetTuning().StartingPlateType;
    }
    const ADThrowable* Item = GetGrabbedThrowable();
    return Item ? Item->GetThrowableType() : NAME_None;
}

float ADPawn::GetThrowCharge() const
{
    if (!PendingThrow)
    {
        return 0.f;
    }
    const float ChargeTime = GetTuning().ThrowChargeTime;
    return ChargeTime > 0.f
        ? FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - PendingThrow->StartedAt) / ChargeTime, 0.f, 1.f)
        : 1.f;
}

void ADPawn::GrabNearbyThrowable()
{
    const FVector Center = Capsule->GetComponentLocation();
    const FVector Up = Capsule->GetUpVector();
    const double Radius = Capsule->GetScaledCapsuleRadius();
    const double HalfSegment = Capsule->GetScaledCapsuleHalfHeight() - Radius;
    const double MaxGap = GetTuning().GrabDistance;
    ADThrowable* Nearest = nullptr;
    double NearestGap = MaxGap;
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(DelerictGrab), false, this);
    for (TActorIterator<ADThrowable> It(GetWorld()); It; ++It)
    {
        ADThrowable* Item = *It;
        if (!Item->CanGrab() || !CanCarryThrowable(Item->GetThrowableType()))
        {
            continue;
        }

        const FVector Offset = Item->GetActorLocation() - Center;
        const FVector FromSegment = Offset - Up * FMath::Clamp(FVector::DotProduct(Offset, Up), -HalfSegment, HalfSegment);
        const double Gap = FMath::Max(0., FromSegment.Size() - Radius - Item->GetCollisionRadius());
        if (Gap > MaxGap || (Nearest && Gap >= NearestGap))
        {
            continue;
        }
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, Center, Item->GetActorLocation(), ECC_Visibility, Query)
            || Hit.GetActor() == Item)
        {
            Nearest = Item;
            NearestGap = Gap;
        }
    }

    if (Nearest)
    {
        TryGrabThrowable(*Nearest);
    }
}

bool ADPawn::CanCarryThrowable(FName Type) const
{
    const int32* Limit = GetTuning().ThrowableCarryLimits.Find(Type);
    return GetThrowableMass(Type) > 0.f && (!Limit || GetThrowableCountByType(Type) < *Limit);
}

bool ADPawn::TryGrabThrowable(ADThrowable& Item)
{
    const FName Type = Item.GetThrowableType();
    if (Item.GetWorld() != GetWorld() || !Item.CanGrab() || !CanCarryThrowable(Type))
    {
        return false;
    }
    const FVector ItemVelocity = UPlanarConstraintUtils::ConstrainDirectionToPlane(
        Mover->GetPlanarConstraint(), Item.GetVelocity());
    const FVector PlayerVelocity = GetVelocity() + GetQueuedVelocityChange();
    if (!Item.TryHold(*this))
    {
        return false;
    }

    if (RemoteThrowable.Get() == &Item)
    {
        RemoteThrowable.Reset();
    }
    GrabbedThrowables.Add(&Item);
    // With the item now counted in carried mass, this impulse gives the combined body its shared drift.
    const FVector Separation = ItemVelocity - PlayerVelocity;
    QueueImpulse(Separation * GetThrowableMass(Type));
#if DELERICT_WITH_GAMEPLAY_DEBUG
    RecordThrowResult(TEXT("Grabbed"));
#endif
    return true;
}

void ADPawn::CycleSelectedThrowable()
{
    TArray<FName, TInlineAllocator<8>> AvailableTypes;
    if (PlateCount > 0)
    {
        AvailableTypes.Add(GetTuning().StartingPlateType);
    }
    for (const ADThrowable* Item : GrabbedThrowables)
    {
        if (IsValid(Item))
        {
            AvailableTypes.AddUnique(Item->GetThrowableType());
        }
    }
    if (!AvailableTypes.IsEmpty())
    {
        const int32 CurrentIndex = AvailableTypes.IndexOfByKey(GetSelectedThrowableType());
        SelectThrowableType(AvailableTypes[(CurrentIndex + 1) % AvailableTypes.Num()]);
    }
}

void ADPawn::SelectThrowableType(FName Type)
{
    if (Type != GetSelectedThrowableType())
    {
        CancelThrowCharge();
    }
    SelectedThrowableType = Type;
}

void ADPawn::HandleThrowCharge(EDChargePhase Phase, const FVector& Aim)
{
    if (Phase == EDChargePhase::Pressed)
    {
        const FName Type = GetSelectedThrowableType();
        if (GetThrowableMass(Type) > 0.f && GetThrowableCountByType(Type) > 0)
        {
            PendingThrow = FThrowCharge{ GetWorld()->GetTimeSeconds(), Type };
        }
#if DELERICT_WITH_GAMEPLAY_DEBUG
        RecordThrowResult(PendingThrow ? TEXT("Charging") : TEXT("Selection empty"));
#endif
        return;
    }

    if (PendingThrow)
    {
        const float Charge = GetThrowCharge();
        const FName ChargedType = PendingThrow->Type;
        PendingThrow.Reset();
        if (Phase == EDChargePhase::Released && ChargedType == GetSelectedThrowableType())
        {
            LaunchSelectedThrowable(Aim, Charge);
        }
#if DELERICT_WITH_GAMEPLAY_DEBUG
        else
        {
            RecordThrowResult(TEXT("Canceled"));
        }
#endif
    }
}

void ADPawn::LaunchSelectedThrowable(const FVector& Aim, float Charge)
{
    const FName Type = GetSelectedThrowableType();
    const float ItemMass = GetThrowableMass(Type);
    if (ItemMass <= 0.f || Aim.IsNearlyZero())
    {
        return;
    }

    const bool bUseStartingPlate = PlateCount > 0 && Type == GetTuning().StartingPlateType;
    const int32 GrabbedIndex = bUseStartingPlate ? INDEX_NONE : GrabbedThrowables.IndexOfByPredicate([Type](const ADThrowable* Item)
    {
        return IsValid(Item) && Item->GetThrowableType() == Type;
    });
    if (!bUseStartingPlate && GrabbedIndex == INDEX_NONE)
    {
        return;
    }

    ADThrowable* Item = bUseStartingPlate ? nullptr : GrabbedThrowables[GrabbedIndex].Get();
    const ADThrowable* ItemShape = Item ? Item : GetDefault<ADThrowable>();
    const double ItemRadius = ItemShape->GetCollisionRadius();
    const double CapsuleRadius = Capsule->GetScaledCapsuleRadius();
    const double CapsuleHalfSegment = Capsule->GetScaledCapsuleHalfHeight() - CapsuleRadius;
    const double CapsuleExtent = CapsuleRadius + CapsuleHalfSegment * FMath::Abs(FVector::DotProduct(Aim, Capsule->GetUpVector()));
    const FVector Origin = Capsule->GetComponentLocation();
    const FVector LaunchLocation = Origin + Aim * (CapsuleExtent + ItemRadius + 2.);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DelerictThrow), false, this);
    Query.AddIgnoredActor(Item);
    UWorld* World = GetWorld();
    if (World->SweepTestByProfile(Origin, LaunchLocation, FQuat::Identity, TEXT("BlockAllDynamic"),
        FCollisionShape::MakeSphere(ItemRadius), Query))
    {
#if DELERICT_WITH_GAMEPLAY_DEBUG
        RecordThrowResult(TEXT("Launch blocked"));
#endif
        return;
    }

    if (!Item)
    {
        Item = World->SpawnActor<ADThrowable>(LaunchLocation, FRotator::ZeroRotator);
        if (!IsValid(Item))
        {
            return;
        }
        Item->SetThrowableType(Type);
    }

    const FVector Drift = GetVelocity() + GetQueuedVelocityChange();
    if (GrabbedIndex != INDEX_NONE)
    {
        GrabbedThrowables.RemoveAt(GrabbedIndex);
    }
    else
    {
        --PlateCount;
    }
    if (GetThrowableCountByType(Type) == 0)
    {
        SelectedThrowableType = NAME_None;
    }

    const UDPlayerTuning& Tuning = GetTuning();
    const double TapSpeed = FMath::Max(0.f, Tuning.MinThrowSeparationSpeed);
    const double FullSpeed = FMath::Max(TapSpeed, static_cast<double>(Tuning.MaxThrowSeparationSpeed));
    const double SeparationSpeed = FMath::Lerp(TapSpeed, FullSpeed, Charge);
    const double RemainingMass = GetEffectiveMass();
    // Solve for the equal and opposite impulses that produce the requested relative speed.
    const FVector Impulse = Aim * (SeparationSpeed / (1. / RemainingMass + 1. / ItemMass));
    const FVector ItemVelocity = Drift + Impulse / ItemMass;
    QueueImpulse(-Impulse);
    if (Item->bRemoteActivatable)
    {
        RemoteThrowable = Item;
    }
    Item->Launch(LaunchLocation, ItemVelocity, Mover->GetPlanarConstraint().PlaneConstraintNormal, this);
#if DELERICT_WITH_GAMEPLAY_DEBUG
    RecordThrowResult(TEXT("Launched"), ItemVelocity, -Impulse / RemainingMass);
#endif
}

void ADPawn::ActivateThrownItem()
{
    if (ADThrowable* Item = RemoteThrowable.Get(); Item && Item->TryActivate(*this))
    {
#if DELERICT_WITH_GAMEPLAY_DEBUG
        RecordThrowResult(TEXT("Remote activation"));
#endif
        return;
    }
    RemoteThrowable.Reset();
}

#if DELERICT_WITH_GAMEPLAY_DEBUG
void ADPawn::RecordThrowResult(const TCHAR* Result, const FVector& ItemVelocity, const FVector& Recoil)
{
    if (DGameplayDebug::IsEnabled(GetWorld(), DGameplayDebug::EChannel::Throws))
    {
        DebugThrowResult = Result;
        DebugItemVelocity = ItemVelocity;
        DebugThrowRecoil = Recoil;
    }
}

void ADPawn::DrawThrowDiagnostics() const
{
    using namespace DGameplayDebug;
    if (IsEnabled(GetWorld(), EChannel::Throws))
    {
        const FName SelectedType = GetSelectedThrowableType();
        const FVector Location = GetActorLocation();
        DrawLabel(GetWorld(), EChannel::Throws, Location + FVector(0., 0., 177.), FString::Printf(
            TEXT("%s x%d | starting plates %d | pickups %d | charge %.0f%% | %s | item velocity %s cm/s | recoil %s cm/s"),
            *SelectedType.ToString(), GetThrowableCountByType(SelectedType), PlateCount, GetPickedUpThrowableCount(),
            100.f * GetThrowCharge(), *DebugThrowResult, *DebugItemVelocity.ToCompactString(), *DebugThrowRecoil.ToCompactString()));
        DrawArrow(GetWorld(), EChannel::Throws, Location, Location + (DebugItemVelocity * .1).GetClampedToMaxSize(140.));
        DrawArrow(GetWorld(), EChannel::Throws, Location, Location + DebugThrowRecoil);
    }
}
#endif
