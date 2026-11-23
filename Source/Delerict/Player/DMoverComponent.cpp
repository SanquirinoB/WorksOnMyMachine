#include "DMoverComponent.h"

#include "Backends/MoverStandaloneLiaison.h"

UDMoverComponent::UDMoverComponent()
{
    BackendClass = UMoverStandaloneLiaisonComponent::StaticClass();
}

void UDMoverComponent::PostInitProperties()
{
    Super::PostInitProperties();
    if (IsTemplate())
    {
        // Blueprint validation checks mode settings before Mover's normal runtime initialization.
        RefreshSharedSettings();
    }
}
