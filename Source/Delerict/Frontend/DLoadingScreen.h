#pragma once

#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DLoadingScreen.generated.h"

UCLASS()
class UDLoadingScreen : public UUserWidget
{
    GENERATED_BODY()

public:
    static TSharedRef<SWidget> MakeScreen();

protected:
    TSharedRef<SWidget> RebuildWidget() override;
};

UCLASS()
class UDLoadingScreenSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    void Deinitialize() override;

private:
    void PrepareMap(const FWorldContext& Context, const FString& MapName);
    void HandleGameStart(UGameInstance* Game);
    void ShowViewportScreen();
    void ReleaseViewportScreen();

    FGuid ViewportRequest;
    FTSTicker::FDelegateHandle ReleaseTicker;
    bool bPreparedFirstMap = false;
};
