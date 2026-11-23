#pragma once

#include "GameFramework/HUD.h"
#include "DCrosshairHUD.generated.h"

UCLASS()
class ADCrosshairHUD : public AHUD
{
    GENERATED_BODY()

    void DrawHUD() override;
};
