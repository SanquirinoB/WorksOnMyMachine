#include "WorksOnMyMachineGameMode.h"

#include "WorksOnMyMachinePawn.h"

AWorksOnMyMachineGameMode::AWorksOnMyMachineGameMode()
{
    DefaultPawnClass = AWorksOnMyMachinePawn::StaticClass();
}
