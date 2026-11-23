#include "DPawn.h"

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

const ADThrowable* ADPawn::GetGrabbedThrowable() const
{
    return IsValid(GrabbedThrowable) ? GrabbedThrowable.Get() : nullptr;
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
    const ADThrowable* Held = GetGrabbedThrowable();
    const float HeldMass = Held ? GetThrowableMass(Held->GetThrowableType()) : 0.f;
    return BodyMass + PlateCount * GetThrowableMass(GetTuning().StartingPlateType) + HeldMass;
}

FName ADPawn::GetSelectedThrowableType() const
{
    if (bGrabbedThrowableSelected)
    {
        const ADThrowable* Held = GetGrabbedThrowable();
        return Held ? Held->GetThrowableType() : NAME_None;
    }
    return PlateCount > 0 ? GetTuning().StartingPlateType : NAME_None;
}

float ADPawn::GetThrowCharge() const
{
    if (!ThrowChargeStartedAt)
    {
        return 0.f;
    }
    const float ChargeTime = GetTuning().ThrowChargeTime;
    return ChargeTime > 0.f
        ? FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - *ThrowChargeStartedAt) / ChargeTime, 0.f, 1.f)
        : 1.f;
}

void ADPawn::GrabNearbyThrowable()
{
    if (GetGrabbedThrowable())
    {
        return;
    }

    const FVector Center = Capsule->GetComponentLocation();
    const FVector Up = Capsule->GetUpVector();
    const double Radius = Capsule->GetScaledCapsuleRadius();
    const double HalfSegment = Capsule->GetScaledCapsuleHalfHeight() - Radius;
    const double MaxGap = GetTuning().GrabDistance;
    ADThrowable* Nearest = nullptr;
    double NearestGap = MaxGap;
    for (TActorIterator<ADThrowable> It(GetWorld()); It; ++It)
    {
        ADThrowable* Item = *It;
        if (!Item->CanGrab() || GetThrowableMass(Item->GetThrowableType()) <= 0.f)
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
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DelerictGrab), false, this);
        Query.AddIgnoredActor(Item);
        if (!GetWorld()->LineTraceTestByChannel(Center, Item->GetActorLocation(), ECC_Visibility, Query))
        {
            Nearest = Item;
            NearestGap = Gap;
        }
    }

    if (!Nearest)
    {
        return;
    }

    const FVector ItemVelocity = UPlanarConstraintUtils::ConstrainDirectionToPlane(
        Mover->GetPlanarConstraint(), Nearest->GetVelocity());
    const FVector PlayerVelocity = GetVelocity() + GetQueuedVelocityChange();
    const float ItemMass = GetThrowableMass(Nearest->GetThrowableType());
    if (!Nearest->TryHold(*this))
    {
        return;
    }

    if (bGrabbedThrowableSelected || PlateCount == 0)
    {
        CancelThrowCharge();
        bGrabbedThrowableSelected = true;
    }
    if (RemoteThrowable.Get() == Nearest)
    {
        RemoteThrowable.Reset();
    }
    GrabbedThrowable = Nearest;
    // With the item now counted in carried mass, this impulse gives the combined body its shared drift.
    const FVector Separation = ItemVelocity - PlayerVelocity;
    QueueImpulse(Separation * ItemMass);
#if DELERICT_WITH_GAMEPLAY_DEBUG
    RecordThrowResult(TEXT("Grabbed"));
#endif
}

void ADPawn::CycleSelectedThrowable()
{
    const bool bHasGrabbed = GetGrabbedThrowable() != nullptr;
    const bool bHasPlates = PlateCount > 0;
    if (!bHasGrabbed && !bHasPlates)
    {
        return;
    }
    const bool bSelectGrabbed = bHasGrabbed && (!bHasPlates || !bGrabbedThrowableSelected);
    if (bSelectGrabbed != bGrabbedThrowableSelected)
    {
        CancelThrowCharge();
        bGrabbedThrowableSelected = bSelectGrabbed;
    }
}

void ADPawn::HandleThrowCharge(EDChargePhase Phase, const FVector& Aim)
{
    if (Phase == EDChargePhase::Pressed)
    {
        if (GetThrowableMass(GetSelectedThrowableType()) > 0.f)
        {
            ThrowChargeStartedAt = GetWorld()->GetTimeSeconds();
        }
#if DELERICT_WITH_GAMEPLAY_DEBUG
        RecordThrowResult(ThrowChargeStartedAt ? TEXT("Charging") : TEXT("Selection empty"));
#endif
        return;
    }

    if (ThrowChargeStartedAt)
    {
        const float Charge = GetThrowCharge();
        ThrowChargeStartedAt.Reset();
        if (Phase == EDChargePhase::Released)
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

    ADThrowable* Item = bGrabbedThrowableSelected ? GrabbedThrowable.Get() : nullptr;
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
    if (bGrabbedThrowableSelected)
    {
        GrabbedThrowable = nullptr;
        bGrabbedThrowableSelected = PlateCount == 0;
    }
    else
    {
        --PlateCount;
        bGrabbedThrowableSelected = PlateCount == 0 && GetGrabbedThrowable();
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
        const ADThrowable* Held = GetGrabbedThrowable();
        const FVector Location = GetActorLocation();
        DrawLabel(GetWorld(), EChannel::Throws, Location + FVector(0., 0., 177.), FString::Printf(
            TEXT("%s | plates %d | held %.2f kg | charge %.0f%% | %s | item velocity %s cm/s | recoil %s cm/s"),
            *GetSelectedThrowableType().ToString(), PlateCount, Held ? GetThrowableMass(Held->GetThrowableType()) : 0.f,
            100.f * GetThrowCharge(), *DebugThrowResult, *DebugItemVelocity.ToCompactString(), *DebugThrowRecoil.ToCompactString()));
        DrawArrow(GetWorld(), EChannel::Throws, Location, Location + (DebugItemVelocity * .1).GetClampedToMaxSize(140.));
        DrawArrow(GetWorld(), EChannel::Throws, Location, Location + DebugThrowRecoil);
    }
}
#endif
