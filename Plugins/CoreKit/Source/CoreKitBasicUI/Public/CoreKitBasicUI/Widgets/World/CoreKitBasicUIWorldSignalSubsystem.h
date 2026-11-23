#pragma once
#include "Subsystems/WorldSubsystem.h"
#include "CoreKitBasicUIWorldSignalSubsystem.generated.h"
class UCoreKitBasicUIEnemySignalComponent;
class UCoreKitBasicUIWorldSignalLayer;
class ULocalPlayer;
/** Opt-in, presentation-only registry. Each local player owns a separate projected layer. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIWorldSignalSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    bool ShouldCreateSubsystem(UObject* Outer) const override;
    void Deinitialize() override;
    void Tick(float DeltaTime) override;
    TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCoreKitBasicUIWorldSignalSubsystem, STATGROUP_Tickables); }
    bool IsTickableWhenPaused() const override { return true; }
    void Register(UCoreKitBasicUIEnemySignalComponent* Signal);
    void Unregister(UCoreKitBasicUIEnemySignalComponent* Signal);
    const TArray<TWeakObjectPtr<UCoreKitBasicUIEnemySignalComponent>>& GetSignals() const { return Signals; }
    static bool IsEnabled();
private:
    TArray<TWeakObjectPtr<UCoreKitBasicUIEnemySignalComponent>> Signals;
    UPROPERTY(Transient) TMap<TObjectPtr<ULocalPlayer>, TObjectPtr<UCoreKitBasicUIWorldSignalLayer>> Layers;
};
