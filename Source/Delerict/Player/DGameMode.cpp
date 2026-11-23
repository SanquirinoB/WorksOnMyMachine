#include "DGameMode.h"

#include "DPawn.h"

ADGameMode::ADGameMode()
{
    DefaultPawnClass = ADPawn::StaticClass();
}
