#pragma once

#include "GameFramework/HUD.h"
#include "DCrosshairHUD.generated.h"

class UDInventoryHUD;

UCLASS()
class ADCrosshairHUD : public AHUD
{
    GENERATED_BODY()

    void PostRender() override;
    void DrawHUD() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(Transient) TObjectPtr<UDInventoryHUD> Inventory;
};
