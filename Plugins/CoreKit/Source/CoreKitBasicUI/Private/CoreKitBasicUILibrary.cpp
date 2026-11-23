#include "CoreKitBasicUI/CoreKitBasicUILibrary.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitBasicUI/Widgets/FrontEnd/CoreKitBasicUIFrontEndMenus.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

UCoreKitBasicUIContextSubsystem* UCoreKitBasicUILibrary::GetUIContext(APlayerController* PC)
{
    ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
    return Player ? Player->GetSubsystem<UCoreKitBasicUIContextSubsystem>() : nullptr;
}
UCoreKitBasicUIMainMenu* UCoreKitBasicUILibrary::OpenMainMenu(APlayerController* PC)
{
    ULocalPlayer* Player = PC ? PC->GetLocalPlayer() : nullptr;
    auto* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
    if (!Nav) return nullptr;
    if (auto* Existing = Cast<UCoreKitBasicUIMainMenu>(Nav->GetTopMenu())) return Existing;
    auto* Menu = CreateWidget<UCoreKitBasicUIMainMenu>(PC, UCoreKitBasicUIMainMenu::StaticClass());
    if (Menu) Nav->PushMenu(Menu, TEXT("Menu"));
    return Menu;
}
UCoreKitBasicUISettingsMenu* UCoreKitBasicUILibrary::OpenSettings(APlayerController* PC) { return CoreKitBasicUIFrontEnd::OpenSettings(PC); }
UCoreKitBasicUIHowToPlayMenu* UCoreKitBasicUILibrary::OpenHowToPlay(APlayerController* PC) { return CoreKitBasicUIFrontEnd::OpenHowToPlay(PC); }
UCoreKitBasicUICreditsMenu* UCoreKitBasicUILibrary::OpenCredits(APlayerController* PC) { return CoreKitBasicUIFrontEnd::OpenCredits(PC); }
