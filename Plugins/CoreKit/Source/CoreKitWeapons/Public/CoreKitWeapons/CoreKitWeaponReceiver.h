#pragma once

#include "CoreKitWeapons/CoreKitWeaponTypes.h"
#include "UObject/Interface.h"
#include "CoreKitWeaponReceiver.generated.h"

UINTERFACE(BlueprintType)
class UCoreKitWeaponReceiver : public UInterface
{
    GENERATED_BODY()
};

class COREKITWEAPONS_API ICoreKitWeaponReceiver
{
    GENERATED_BODY()

public:
    /** Return true if the hit was accepted. The receiver owns health, team rules and reactions. */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="CoreKit|Weapons")
    bool ReceiveWeaponImpact(const FCoreKitWeaponImpact& Impact);
};
