#include "WorksOnMyMachineMoverComponent.h"

#include "Backends/MoverStandaloneLiaison.h"

UWorksOnMyMachineMoverComponent::UWorksOnMyMachineMoverComponent()
{
    BackendClass = UMoverStandaloneLiaisonComponent::StaticClass();
}

void UWorksOnMyMachineMoverComponent::PostInitProperties()
{
    Super::PostInitProperties();
    if (IsTemplate())
    {
        // Blueprint validation checks mode settings before Mover's normal runtime initialization.
        RefreshSharedSettings();
    }
}
