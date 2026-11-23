#pragma once
#include "Components/ActorComponent.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalTypes.h"
#include "CoreKitBasicUIEnemySignalComponent.generated.h"

/** Attach to any actor and push health/charge from the owning game. Never reads or modifies combat. */
UCLASS(ClassGroup=(CoreKit), meta=(BlueprintSpawnableComponent))
class COREKITBASICUI_API UCoreKitBasicUIEnemySignalComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoreKitBasicUIEnemySignalComponent();
    AActor* GetEnemy() const { return GetOwner(); }
    bool IsBoss() const { return bBoss; }
    bool IsAlive() const { return bAlive; }
    bool WantsHealthBar() const { return bDamaged && !bBoss && bAlive; }
    float GetSecondsSinceHit() const;
    const FCoreKitBasicUILagBar& GetBar() const { return Bar; }
    void TickPresentation(float RealDeltaSeconds);
    bool IsCharging() const { return bChargeVisible && bAlive; }
    float GetChargeProgress() const { return ChargeProgress; }
    float GetTopOffset() const { return TopOffset; }
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") void SetHealth(float Current, float Maximum);
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") void SetCharge(bool bVisible, float Progress);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bBoss = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float TopOffset = 100.f;
protected:
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    FCoreKitBasicUILagBar Bar;
    double LastHitRealTime = -1000.;
    double LastTickRealTime = 0.;
    bool bDamaged = false;
    bool bAlive = true;
    bool bChargeVisible = false;
    float ChargeProgress = 0.f;
};
