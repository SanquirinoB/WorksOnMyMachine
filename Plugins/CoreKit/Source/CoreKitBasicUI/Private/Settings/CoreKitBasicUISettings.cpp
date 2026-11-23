#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "InputAction.h"

UInputAction* UCoreKitBasicUISettings::FindAction(FName Role) const
{
    const TSoftObjectPtr<UInputAction>* Action = InputActions.Find(Role);
    return Action ? Action->LoadSynchronous() : nullptr;
}

FText UCoreKitBasicUISettings::ResolveText(FName Key, const FText& Fallback)
{
    const FText* Override = GetDefault<UCoreKitBasicUISettings>()->TextOverrides.Find(Key);
    return Override ? *Override : Fallback;
}
