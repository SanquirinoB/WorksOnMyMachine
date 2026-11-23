#pragma once

#include "GameFramework/GameModeBase.h"
#include "DTitleGameMode.generated.h"

UCLASS()
class ADTitleGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADTitleGameMode();

protected:
    void BeginPlay() override;
};
