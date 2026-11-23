#include "DMagneticSurfaceMode.h"

#include "Components/CapsuleComponent.h"
#include "DMoverComponent.h"
#include "DPawn.h"
#include "DPlayerTuning.h"
#include "DSteelSurfaceComponent.h"
#include "Engine/ScopedMovementUpdate.h"
#include "Engine/World.h"
#include "MoveLibrary/ConstrainedMoveUtils.h"
#include "MoverDataModelTypes.h"
#include "UObject/UObjectIterator.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

const FName UDMagneticSurfaceMode::ModeName(TEXT("MagneticSurface"));

namespace
{
    constexpr double ContactTolerance = .25;

    double GetCapsuleSupport(const UCapsuleComponent& Capsule, const FVector& Normal)
    {
        const double Radius = Capsule.GetScaledCapsuleRadius();
        const double AxialHalfLength = Capsule.GetScaledCapsuleHalfHeight() - Radius;
        return Radius + AxialHalfLength * FMath::Abs(FVector::DotProduct(Capsule.GetUpVector(), Normal));
    }
}

void UDFlyingMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
    auto& Mover = GetMoverComponentChecked<UDMoverComponent>();
    auto* Magnetic = Cast<UDMagneticSurfaceMode>(Mover.FindMovementModeByName(UDMagneticSurfaceMode::ModeName));
    if (Magnetic && Magnetic->TryCapture(Params, OutputState))
    {
        return;
    }
    Super::SimulationTick_Implementation(Params, OutputState);
}

UDMagneticSurfaceMode::UDMagneticSurfaceMode()
{
    GameplayTags.AddTag(Mover_IsOnGround);
}

ADPawn* UDMagneticSurfaceMode::GetPawn() const
{
    const auto* Mover = GetMoverComponent<UDMoverComponent>();
    return Mover ? Cast<ADPawn>(Mover->GetOwner()) : nullptr;
}

bool UDMagneticSurfaceMode::SampleFace(const UDSteelSurfaceComponent& Face, const UCapsuleComponent& Capsule,
    const FPlanarConstraint& Plane, const FVector& Location, FContact& Contact)
{
    if (!Face.CanSupport(Capsule))
    {
        return false;
    }

    const FVector Normal = Face.GetUpVector();
    const FVector LaneNormal = Plane.PlaneConstraintNormal;
    if (FMath::Abs(FVector::DotProduct(Normal, LaneNormal)) > .001)
    {
        return false;
    }

    const double Distance = FVector::DotProduct(Location - Face.GetComponentLocation(), Normal);
    const FVector Point = Location - Normal * Distance;
    if (!Face.ContainsProjection(Point, ContactTolerance))
    {
        return false;
    }

    Contact.Point = Point;
    Contact.Normal = Normal;
    Contact.Tangent = FVector::CrossProduct(LaneNormal, Normal).GetSafeNormal();
    Contact.CapsuleSupport = GetCapsuleSupport(Capsule, Normal);
    Contact.Gap = Distance - Contact.CapsuleSupport;
    return true;
}

bool UDMagneticSurfaceMode::SweepToContact(const FSimulationTickParams& Params, const FVector& Target)
{
    USceneComponent& Updated = *Params.MovingComps.UpdatedComponent.Get();
    const FVector Delta = Target - Updated.GetComponentLocation();
    if (Delta.IsNearlyZero())
    {
        return true;
    }

    // A failed snap or carry must leave the capsule where Flying started the step.
    FScopedMovementUpdate Update(&Updated, EScopedUpdate::DeferredUpdates);
    FHitResult Hit;
    Updated.MoveComponent(Delta, Updated.GetComponentQuat(), true, &Hit);
    if (Hit.bStartPenetrating || !Updated.GetComponentLocation().Equals(Target, ContactTolerance))
    {
        Update.RevertMove();
        return false;
    }
    return true;
}

void UDMagneticSurfaceMode::WriteSyncState(const FSimulationTickParams& Params, FMoverTickEndData& OutputState,
    const FVector& Velocity)
{
    USceneComponent& Updated = *Params.MovingComps.UpdatedComponent.Get();
    auto& Sync = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();
    Sync.SetTransforms_WorldSpace(Updated.GetComponentLocation(), Updated.GetComponentRotation(), Velocity,
        FVector::ZeroVector);
    Sync.MoveDirectionIntent = Params.ProposedMove.bHasDirIntent ? Params.ProposedMove.DirectionIntent : FVector::ZeroVector;
    Updated.ComponentVelocity = Velocity;
}

void UDMagneticSurfaceMode::RearmFace(const UCapsuleComponent& Capsule, double RecaptureDistance)
{
    const auto* Face = DepartedFace.Get();
    if (!Face || !Face->CanSupport(Capsule))
    {
        DepartedFace.Reset();
        return;
    }
    const FVector ToCapsule = Capsule.GetComponentLocation() - Face->ClosestPoint(Capsule.GetComponentLocation());
    const double Gap = ToCapsule.Size() - GetCapsuleSupport(Capsule, ToCapsule.GetSafeNormal());
    if (Gap >= RecaptureDistance)
    {
        DepartedFace.Reset();
    }
}

bool UDMagneticSurfaceMode::TryCapture(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
    const auto* Pawn = GetPawn();
    const auto* Capsule = Cast<UCapsuleComponent>(Params.MovingComps.UpdatedPrimitive.Get());
    if (!Pawn || !Capsule)
    {
        return false;
    }

    const UDPlayerTuning& Tuning = Pawn->GetTuning();
    RearmFace(*Capsule, FMath::Max(Tuning.RecaptureDistance, Tuning.CaptureDistance + 1.f));
    if (!Pawn->IsMagnetHeld() || LastDepartureTimeMs == Params.TimeStep.BaseSimTimeMs)
    {
        return false;
    }

    const FPlanarConstraint& Plane = GetMoverComponentChecked<UDMoverComponent>().GetPlanarConstraint();
    const FVector Location = Capsule->GetComponentLocation();
    struct FCandidate
    {
        UDSteelSurfaceComponent* Face;
        FContact Contact;
    };
    TArray<FCandidate, TInlineAllocator<4>> Candidates;
    for (TObjectIterator<UDSteelSurfaceComponent> It; It; ++It)
    {
        UDSteelSurfaceComponent* Face = *It;
        FContact Contact;
        if (Face->GetWorld() == Pawn->GetWorld() && Face != DepartedFace.Get()
            && SampleFace(*Face, *Capsule, Plane, Location, Contact)
            && Contact.Gap >= -ContactTolerance && Contact.Gap <= Tuning.CaptureDistance)
        {
            Candidates.Add({ Face, Contact });
        }
    }
    Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Contact.Gap < B.Contact.Gap; });

    const auto* StartSync = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
    check(StartSync);
    for (const FCandidate& Candidate : Candidates)
    {
        const FContact& Contact = Candidate.Contact;
        const FVector Target = UPlanarConstraintUtils::ConstrainLocationToPlane(Plane,
            Contact.Point + Contact.Normal * (Contact.CapsuleSupport + Tuning.ContactClearance));
        const bool bSnapped = SweepToContact(Params, Target);
#if DELERICT_WITH_GAMEPLAY_DEBUG
        using namespace DGameplayDebug;
        if (IsEnabled(Pawn->GetWorld(), EChannel::Magnet))
        {
            DrawLine(Pawn->GetWorld(), EChannel::Magnet, Location, Target);
            DrawLabel(Pawn->GetWorld(), EChannel::Magnet, Contact.Point, FString::Printf(
                TEXT("%s | gap %.1f / %.1f cm"), bSnapped ? TEXT("Captured") : TEXT("Snap blocked"),
                Contact.Gap, Tuning.CaptureDistance), .4f);
        }
#endif
        if (!bSnapped)
        {
            continue;
        }

        Surface = Candidate.Face;
        ContactLocal = Candidate.Face->GetComponentTransform().InverseTransformPosition(Contact.Point);
        SurfaceNormal = Contact.Normal;
        SurfaceTangent = Contact.Tangent;
        RelativeSpeed = FVector::DotProduct(StartSync->GetVelocity_WorldSpace(), SurfaceTangent);
        const double Facing = FMath::IsNearlyZero(RelativeSpeed)
            ? FVector::DotProduct(Pawn->GetAimDirection(), SurfaceTangent) : RelativeSpeed;
        FacingSign = Facing < 0. ? -1.f : 1.f;
        LastCarryVelocity = FVector::ZeroVector;
        LastWorldVelocity = SurfaceTangent * RelativeSpeed;
        WriteSyncState(Params, OutputState, LastWorldVelocity);
        OutputState.MovementEndState.NextModeName = ModeName;
        OutputState.MovementEndState.RemainingMs = Params.TimeStep.StepMs;
        return true;
    }
    return false;
}

void UDMagneticSurfaceMode::GenerateMove_Implementation(const FMoverSimContext& /*SimContext*/,
    const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
    const auto* Pawn = GetPawn();
    const auto* Face = Surface.Get();
    const auto* StartSync = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
    if (!Pawn || !Face || !StartSync)
    {
        return;
    }

    const UDPlayerTuning& Tuning = Pawn->GetTuning();
    const FVector Tangent = FVector::CrossProduct(
        GetMoverComponentChecked<UDMoverComponent>().GetPlanarConstraint().PlaneConstraintNormal,
        Face->GetUpVector()).GetSafeNormal();
    const FVector2D Move = Pawn->GetMoveIntent();
    const float Input = FMath::Clamp(FVector::DotProduct(FVector(Move.X, 0., Move.Y), Tangent), -1., 1.);
    const float Target = Input * (Pawn->IsRunHeld() ? Tuning.SurfaceRunSpeed : Tuning.SurfaceWalkSpeed);
    const float Current = FVector::DotProduct(StartSync->GetVelocity_WorldSpace() - LastCarryVelocity, Tangent);
    const bool bSpeedingUp = Target * Current >= 0.f && FMath::Abs(Target) > FMath::Abs(Current);
    const float Rate = bSpeedingUp ? Tuning.SurfaceAcceleration : Tuning.SurfaceBraking;
    // FInterpConstantTo treats zero as an immediate snap. Zero acceleration or braking here preserves drift.
    const float Speed = Rate > 0.f ? FMath::FInterpConstantTo(Current, Target, TimeStep.StepMs * .001f, Rate) : Current;
    OutProposedMove.LinearVelocity = Tangent * Speed;
    OutProposedMove.DirectionIntent = Tangent * Input;
    OutProposedMove.bHasDirIntent = !FMath::IsNearlyZero(Input);
}

void UDMagneticSurfaceMode::Depart(const FSimulationTickParams& Params, FMoverTickEndData& OutputState,
    const FVector& Velocity, const TCHAR* Reason, bool bRefundTime)
{
    DepartedFace = Surface;
    Surface.Reset();
    LastDepartureTimeMs = Params.TimeStep.BaseSimTimeMs;
    WriteSyncState(Params, OutputState, Velocity);
    OutputState.MovementEndState.NextModeName = DefaultModeNames::Flying;
    OutputState.MovementEndState.RemainingMs = bRefundTime ? Params.TimeStep.StepMs : 0.f;

#if DELERICT_WITH_GAMEPLAY_DEBUG
    using namespace DGameplayDebug;
    if (const auto* Pawn = GetPawn())
    {
        const FVector Location = Pawn->GetActorLocation() + FVector(0., 0., 160.);
        DrawLabel(Pawn->GetWorld(), EChannel::Magnet, Location, Reason, .6f);
        if (JumpStartedAt)
        {
            DrawLabel(Pawn->GetWorld(), EChannel::Jump, Location, TEXT("Charge canceled on departure"), .6f);
        }
    }
#endif
    JumpStartedAt.Reset();
    bJumpDeparturePending = false;
}

void UDMagneticSurfaceMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
    auto* Capsule = Cast<UCapsuleComponent>(Params.MovingComps.UpdatedPrimitive.Get());
    const auto* StartSync = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
    if (!Capsule || !StartSync)
    {
        return;
    }

    const FVector StartVelocity = StartSync->GetVelocity_WorldSpace();
    const auto* Pawn = GetPawn();
    const auto* Face = Surface.Get();
    if (!Pawn || !Face)
    {
        Depart(Params, OutputState, StartVelocity, TEXT("Steel face lost"));
        return;
    }

    const UDPlayerTuning& Tuning = Pawn->GetTuning();
    const FPlanarConstraint& Plane = GetMoverComponentChecked<UDMoverComponent>().GetPlanarConstraint();
    const FTransform& FaceTransform = Face->GetComponentTransform();
    const FVector Anchor = UPlanarConstraintUtils::ConstrainLocationToPlane(Plane, FaceTransform.TransformPosition(ContactLocal));
    FContact Contact;
    if (!SampleFace(*Face, *Capsule, Plane, Anchor, Contact))
    {
        Depart(Params, OutputState, StartVelocity, TEXT("Steel face left the lane"));
        return;
    }
    if (bJumpDeparturePending)
    {
        Depart(Params, OutputState, StartVelocity, TEXT("Jump released"));
        return;
    }
    if (!Pawn->IsMagnetHeld())
    {
        Depart(Params, OutputState, StartVelocity, TEXT("Magnet released"));
        // A fresh hold can recapture this face even if the pawn has not drifted away.
        DepartedFace.Reset();
        return;
    }

    if (FVector::DotProduct(StartVelocity - LastWorldVelocity, Contact.Normal) > Tuning.BreakawayDeltaSpeed)
    {
        Depart(Params, OutputState, StartVelocity, TEXT("Impulse broke contact"));
        return;
    }

    const double Seconds = Params.TimeStep.StepMs * .001;
    const FVector BeforeCarry = Capsule->GetComponentLocation();
    const FVector Target = UPlanarConstraintUtils::ConstrainLocationToPlane(Plane,
        Contact.Point + Contact.Normal * (Contact.CapsuleSupport + Tuning.ContactClearance));
    if (!SweepToContact(Params, Target))
    {
        Depart(Params, OutputState, StartVelocity, TEXT("Platform carry blocked"));
        return;
    }
    const FVector AfterCarry = Capsule->GetComponentLocation();
    LastCarryVelocity = (AfterCarry - BeforeCarry) / Seconds;

    const FVector Delta = Contact.Tangent * FVector::DotProduct(Params.ProposedMove.LinearVelocity, Contact.Tangent) * Seconds;
    FHitResult Hit;
    Capsule->MoveComponent(Delta, Capsule->GetComponentQuat(), true, &Hit);
    const FVector Location = Capsule->GetComponentLocation();
    RelativeSpeed = FVector::DotProduct(Location - AfterCarry, Contact.Tangent) / Seconds;
    LastWorldVelocity = LastCarryVelocity + Contact.Tangent * RelativeSpeed;
    if (!SampleFace(*Face, *Capsule, Plane, Location, Contact) ||
        FMath::Abs(Contact.Gap - Tuning.ContactClearance) > ContactTolerance)
    {
        Depart(Params, OutputState, LastWorldVelocity, TEXT("Left steel face edge"), false);
        return;
    }

    ContactLocal = FaceTransform.InverseTransformPosition(Contact.Point);
    SurfaceNormal = Contact.Normal;
    SurfaceTangent = Contact.Tangent;
    if (!FMath::IsNearlyZero(RelativeSpeed, 1.f))
    {
        FacingSign = FMath::Sign(RelativeSpeed);
    }
    WriteSyncState(Params, OutputState, LastWorldVelocity);

#if DELERICT_WITH_GAMEPLAY_DEBUG
    using namespace DGameplayDebug;
    UWorld* World = Pawn->GetWorld();
    if (IsEnabled(World, EChannel::Magnet))
    {
        DrawArrow(World, EChannel::Magnet, Contact.Point, Contact.Point + Contact.Normal * 55.);
        DrawArrow(World, EChannel::Magnet, Contact.Point, Contact.Point + GetFacingTangent() * 55.);
        DrawLabel(World, EChannel::Magnet, Location + FVector(0., 0., 155.), FString::Printf(
            TEXT("%s | gap %.1f cm | relative %.1f cm/s | carry %s cm/s"),
            Pawn->IsRunHeld() ? TEXT("Run") : TEXT("Walk"), Contact.Gap, GetRelativeSpeed(),
            *LastCarryVelocity.ToCompactString()));
    }
    if (JumpStartedAt && IsEnabled(World, EChannel::Jump))
    {
        DrawLabel(World, EChannel::Jump, Location + FVector(0., 0., 185.), FString::Printf(
            TEXT("Holding %.2f / %.2f s | %.1f deg minimum"), World->GetTimeSeconds() - *JumpStartedAt,
            Tuning.JumpChargeTime, Tuning.MinJumpAngleDegrees));
    }
#endif
}

void UDMagneticSurfaceMode::HandleJumpCharge(EDChargePhase Phase, const FVector& Aim)
{
    ADPawn* Pawn = GetPawn();
    if (!Pawn || !Surface.IsValid() || !Pawn->IsMagnetHeld())
    {
        JumpStartedAt.Reset();
        return;
    }

    const double Now = Pawn->GetWorld()->GetTimeSeconds();
    if (Phase == EDChargePhase::Pressed)
    {
        JumpStartedAt = Now;
        return;
    }
    if (!JumpStartedAt)
    {
        return;
    }

    const double HeldSeconds = Now - *JumpStartedAt;
    JumpStartedAt.Reset();
    if (Phase != EDChargePhase::Released)
    {
#if DELERICT_WITH_GAMEPLAY_DEBUG
        DGameplayDebug::DrawLabel(Pawn->GetWorld(), DGameplayDebug::EChannel::Jump,
            Pawn->GetActorLocation(), TEXT("Charge canceled"), .6f);
#endif
        return;
    }

    const UDPlayerTuning& Tuning = Pawn->GetTuning();
    const double Charge = Tuning.JumpChargeTime > 0.f ? FMath::Clamp(HeldSeconds / Tuning.JumpChargeTime, 0., 1.) : 1.;
    const double Magnitude = FMath::Lerp(static_cast<double>(Tuning.MinJumpImpulseMagnitude),
        static_cast<double>(FMath::Max(Tuning.MinJumpImpulseMagnitude, Tuning.MaxJumpImpulseMagnitude)), Charge);
    const double MinAngle = FMath::Clamp(static_cast<double>(Tuning.MinJumpAngleDegrees), 20., 90.);
    const double MinRadians = FMath::DegreesToRadians(MinAngle);
    const double Along = FVector::DotProduct(Aim, SurfaceTangent);
    const double Outward = FVector::DotProduct(Aim, SurfaceNormal);
    FVector Direction = SurfaceNormal;
    if (!FMath::IsNearlyZero(Along))
    {
        const double Angle = FMath::Max(FMath::Atan2(Outward, FMath::Abs(Along)), MinRadians);
        Direction = SurfaceNormal * FMath::Sin(Angle) + SurfaceTangent * (FMath::Sign(Along) * FMath::Cos(Angle));
    }

    bJumpDeparturePending = true;
    Pawn->QueueImpulse(Direction * Magnitude);
#if DELERICT_WITH_GAMEPLAY_DEBUG
    using namespace DGameplayDebug;
    if (IsEnabled(Pawn->GetWorld(), EChannel::Jump))
    {
        const FVector Location = Pawn->GetActorLocation();
        DrawArrow(Pawn->GetWorld(), EChannel::Jump, Location, Location + Aim * 75.);
        DrawArrow(Pawn->GetWorld(), EChannel::Jump, Location, Location + Direction * 120.);
        DrawLabel(Pawn->GetWorld(), EChannel::Jump, Location, FString::Printf(
            TEXT("%.2f s | %.0f%% | raw %s | launch %s | limit %.1f deg | %.0f kg*cm/s"),
            HeldSeconds, Charge * 100., *Aim.ToCompactString(), *Direction.ToCompactString(), MinAngle, Magnitude), 1.f);
    }
#endif
}
