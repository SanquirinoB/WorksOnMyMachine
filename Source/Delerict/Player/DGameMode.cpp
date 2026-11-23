#include "DGameMode.h"

#include "DCrosshairHUD.h"
#include "DPawn.h"
#include "../LevelDesign/DCheckpointParticipant.h"

ADGameMode::ADGameMode()
{
    DefaultPawnClass = ADPawn::StaticClass();
    HUDClass = ADCrosshairHUD::StaticClass();
    CreateDefaultSubobject<UDCheckpointParticipant>(TEXT("CheckpointSave"));
}
