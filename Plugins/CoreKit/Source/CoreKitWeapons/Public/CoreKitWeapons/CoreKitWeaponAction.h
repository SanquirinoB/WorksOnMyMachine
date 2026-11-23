#pragma once

#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitWeapons/CoreKitWeaponTypes.h"
#include "CoreKitWeaponAction.generated.h"

class UCoreKitWeaponsComponent;

/** CoreKitActions lifetime adapter for a shot, burst, held fire, reload or melee swing. */
UCLASS()
class COREKITWEAPONS_API UCoreKitWeaponAction : public UCoreKitAction
{
    GENERATED_BODY()

public:
    bool EvaluateActivation_Implementation(FName& FailureCode, FString& Detail) const override;

protected:
    void Activate() override;
    void TickTask(float DeltaSeconds) override;
    void CleanupAction(ECoreKitActionEndReason Reason) override;

private:
    friend class UCoreKitWeaponsComponent;
    UCoreKitWeaponsComponent* FindWeapons() const;
    void AdvanceFire();
    void AdvanceMelee(float DeltaSeconds);
    FCoreKitWeaponMeleeStep GetMeleeStep() const;

    TWeakObjectPtr<UCoreKitWeaponsComponent> Weapons;
    TSet<TWeakObjectPtr<AActor>> Victims;
    int32 Slot = INDEX_NONE;
    int32 MeleeStep = 0;
    int32 BurstRemaining = 0;
    double ReloadStarted = 0.;
    double ReloadEnds = 0.;
    float MeleeElapsed = 0.f;
    bool bMeleeBuffered = false;
};
