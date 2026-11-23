#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoreKitBasicUILibrary.generated.h"
class APlayerController;
class UCoreKitBasicUIContextSubsystem;
class UCoreKitBasicUIMainMenu;
class UCoreKitBasicUISettingsMenu;
class UCoreKitBasicUIHowToPlayMenu;
class UCoreKitBasicUICreditsMenu;

/** Small Blueprint entry points for projects with an existing controller and HUD. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUILibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="CoreKit|BasicUI") static UCoreKitBasicUIContextSubsystem* GetUIContext(APlayerController* Controller);
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") static UCoreKitBasicUIMainMenu* OpenMainMenu(APlayerController* Controller);
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") static UCoreKitBasicUISettingsMenu* OpenSettings(APlayerController* Controller);
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") static UCoreKitBasicUIHowToPlayMenu* OpenHowToPlay(APlayerController* Controller);
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") static UCoreKitBasicUICreditsMenu* OpenCredits(APlayerController* Controller);
};
