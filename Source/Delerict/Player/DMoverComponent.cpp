#include "DMoverComponent.h"

#include "Backends/MoverStandaloneLiaison.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

UDMoverComponent::UDMoverComponent()
{
    BackendClass = UMoverStandaloneLiaisonComponent::StaticClass();
    StartingMovementMode = DefaultModeNames::Flying;
}

void UDMoverComponent::InitializeComponent()
{
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
    if (IsTemplate())
    {
        // Blueprint validation checks mode settings before Mover's normal runtime initialization.
        RefreshSharedSettings();
    }
}
