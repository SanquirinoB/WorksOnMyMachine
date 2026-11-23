#include "DGameMode.h"

#include "DCrosshairHUD.h"
#include "DPawn.h"
#include "GameFramework/PlayerStart.h"
#include "../LevelDesign/DCheckpointParticipant.h"

namespace
{
    const FName StartMagnetizedTag(TEXT("Delerict.StartMagnetized"));
}

ADGameMode::ADGameMode()
{
    DefaultPawnClass = ADPawn::StaticClass();
    HUDClass = ADCrosshairHUD::StaticClass();
    CreateDefaultSubobject<UDCheckpointParticipant>(TEXT("CheckpointSave"));
}

APawn* ADGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    APawn* Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
    const auto* PlayerStart = Cast<APlayerStart>(StartSpot);
    if (auto* DelerictPawn = Cast<ADPawn>(Pawn); DelerictPawn && PlayerStart && PlayerStart->PlayerStartTag == StartMagnetizedTag)
    {
        DelerictPawn->RequestInitialMagneticCapture();
    }
    return Pawn;
}
