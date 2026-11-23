#include "DTitleGameMode.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

ADTitleGameMode::ADTitleGameMode()
{
    DefaultPawnClass = nullptr;
}

void ADTitleGameMode::BeginPlay()
{
    Super::BeginPlay();
    const TSoftObjectPtr<USoundWave> Loop(FSoftObjectPath(
        TEXT("/Game/Delerict/Frontend/Delerict_Ambient_Loop_MainMenu.Delerict_Ambient_Loop_MainMenu")));
    if (USoundWave* Track = Loop.LoadSynchronous())
    {
        if (UAudioComponent* Sound = UGameplayStatics::CreateSound2D(this, Track))
        {
            Sound->FadeIn(.4f);
        }
    }
}
