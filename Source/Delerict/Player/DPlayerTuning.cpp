#include "DPlayerTuning.h"

#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    USoundBase* FindPlayerSound(const TCHAR* Name)
    {
        const FString Path = FString::Printf(TEXT("/Game/Delerict/Audio/Player/%s.%s"), Name, Name);
        return ConstructorHelpers::FObjectFinder<USoundBase>(*Path).Object;
    }
}

UDPlayerTuning::UDPlayerTuning(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    BootSteps = { FindPlayerSound(TEXT("SW_DBootStep_01")), FindPlayerSound(TEXT("SW_DBootStep_02")),
        FindPlayerSound(TEXT("SW_DBootStep_03")) };
    BootLatchSound = FindPlayerSound(TEXT("SW_DBootLatch"));
    BootJumpSound = FindPlayerSound(TEXT("SW_DBootJump"));
    ItemGrabSound = FindPlayerSound(TEXT("SW_DItemGrab"));
    ItemThrowSound = FindPlayerSound(TEXT("SW_DItemThrow"));
}
