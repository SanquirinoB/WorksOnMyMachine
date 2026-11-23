#include "DMoverComponent.h"

#include "Backends/MoverStandaloneLiaison.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "DMagneticSurfaceMode.h"
#include "DPawn.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

UDMoverComponent::UDMoverComponent()
{
    BackendClass = UMoverStandaloneLiaisonComponent::StaticClass();
    StartingMovementMode = DefaultModeNames::Flying;
    MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UDFlyingMode>(TEXT("DFlyingMode")));
    MovementModes.Add(UDMagneticSurfaceMode::ModeName,
        CreateDefaultSubobject<UDMagneticSurfaceMode>(TEXT("DMagneticSurfaceMode")));
}

void UDMoverComponent::InitializeComponent()
{
    // Existing player Blueprint templates may carry the mode map saved before magnetic traversal.
    if (!Cast<UDFlyingMode>(FindMovementModeByName(DefaultModeNames::Flying)))
    {
        MovementModes.Add(DefaultModeNames::Flying, NewObject<UDFlyingMode>(this));
    }
    if (!GetMagneticMode())
    {
        MovementModes.Add(UDMagneticSurfaceMode::ModeName, NewObject<UDMagneticSurfaceMode>(this));
    }

    StartingMovementMode = DefaultModeNames::Flying;
    SetHandleJump(false);
    if (UCommonLegacyMovementSettings* Settings = FindSharedSettings_Mutable<UCommonLegacyMovementSettings>())
    {
        Settings->Acceleration = 0.f;
        Settings->Deceleration = 0.f;
        Settings->bShouldRemainVertical = false;
    }

    Super::InitializeComponent();

    if (auto* Backend = GetOwner()->FindComponentByClass<UMoverStandaloneLiaisonComponent>())
    {
        Backend->SetUseAsyncProduceInput(false);
        Backend->SetUseAsyncMovementSimulationTick(false);
    }
}

UDMagneticSurfaceMode* UDMoverComponent::GetMagneticMode() const
{
    return Cast<UDMagneticSurfaceMode>(FindMovementModeByName(UDMagneticSurfaceMode::ModeName));
}

void UDMoverComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Pawn = Cast<ADPawn>(GetOwner()))
    {
        if (auto* Mode = GetMagneticMode())
        {
            Pawn->OnJumpCharge.AddUObject(Mode, &UDMagneticSurfaceMode::HandleJumpCharge);
        }
    }
}

void UDMoverComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* Pawn = Cast<ADPawn>(GetOwner()))
    {
        if (auto* Mode = GetMagneticMode())
        {
            Pawn->OnJumpCharge.RemoveAll(Mode);
        }
    }
    Super::EndPlay(Reason);
}

bool UDMoverComponent::IsMagneticallyAttached() const
{
    const auto* Mode = GetMagneticMode();
    return Mode && Mode->IsAttached() && GetMovementModeName() == UDMagneticSurfaceMode::ModeName;
}

float UDMoverComponent::GetSurfaceTravelSpeed() const
{
    const auto* Mode = GetMagneticMode();
    return Mode ? Mode->GetRelativeSpeed() : 0.f;
}

void UDMoverComponent::UpdateMagneticVisuals(USceneComponent& Mesh, USceneComponent& Placeholder)
{
    const auto* Capsule = Cast<UCapsuleComponent>(GetUpdatedComponent());
    if (!Capsule || !IsMagneticallyAttached())
    {
        return;
    }

    if (!MeshRestPose)
    {
        MeshRestPose = Mesh.GetRelativeTransform();
        PlaceholderRestPose = Placeholder.GetRelativeTransform();
    }
    const auto* Mode = GetMagneticMode();
    const FVector Normal = Mode->GetSurfaceNormal();
    const FQuat SurfaceRotation = FRotationMatrix::MakeFromXZ(Mode->GetFacingTangent(), Normal).ToQuat();
    const double AxialHalfLength = Capsule->GetScaledCapsuleHalfHeight() - Capsule->GetScaledCapsuleRadius();
    // The fixed capsule is shallower against a wall than against a floor. Keep the authored foot clearance.
    const FVector SupportCorrection = Normal * AxialHalfLength
        * (1. - FMath::Abs(FVector::DotProduct(Capsule->GetUpVector(), Normal)));
    const FVector PoseOrigin = Capsule->GetComponentLocation() + SupportCorrection;
    const FVector ParentScale = Capsule->GetComponentScale();
    const auto PlaceVisual = [&SurfaceRotation, &PoseOrigin, &ParentScale](USceneComponent& Visual, const FTransform& Rest)
    {
        Visual.SetWorldLocationAndRotation(PoseOrigin + SurfaceRotation.RotateVector(ParentScale * Rest.GetLocation()),
            SurfaceRotation * Rest.GetRotation());
    };
    PlaceVisual(Mesh, *MeshRestPose);
    PlaceVisual(Placeholder, *PlaceholderRestPose);
}

#if DELERICT_WITH_GAMEPLAY_DEBUG
void UDMoverComponent::OnHandleImpact(const FMoverOnImpactParams& Impact)
{
    Super::OnHandleImpact(Impact);
    if (Impact.MovementModeName == DefaultModeNames::Flying
        && DGameplayDebug::IsEnabled(GetWorld(), DGameplayDebug::EChannel::Collision))
    {
        FlightImpact = Impact.HitResult;
    }
}

TOptional<FHitResult> UDMoverComponent::TakeFlightImpact()
{
    TOptional<FHitResult> Result = MoveTemp(FlightImpact);
    FlightImpact.Reset();
    return Result;
}
#endif

void UDMoverComponent::PostInitProperties()
{
    Super::PostInitProperties();
    const AActor* OwnerTemplate = GetTypedOuter<AActor>();
    if (IsTemplate() && OwnerTemplate && !OwnerTemplate->GetClass()->HasAnyClassFlags(CLASS_Native))
    {
        // Keep generated settings in the Blueprint package so cooked imports have a persistent template.
        RefreshSharedSettings();
    }
}
