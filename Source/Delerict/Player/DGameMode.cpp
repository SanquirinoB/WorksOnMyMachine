#include "DGameMode.h"

#include "DCrosshairHUD.h"
#include "DPawn.h"

ADGameMode::ADGameMode()
{
    DefaultPawnClass = ADPawn::StaticClass();
    HUDClass = ADCrosshairHUD::StaticClass();
}
