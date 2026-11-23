#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIEnemySignalComponent.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

UCoreKitBasicUIEnemySignalComponent::UCoreKitBasicUIEnemySignalComponent() { PrimaryComponentTick.bCanEverTick = false; Bar.Reset(1.f); }
void UCoreKitBasicUIEnemySignalComponent::BeginPlay() { Super::BeginPlay(); if (auto* Signals = GetWorld()->GetSubsystem<UCoreKitBasicUIWorldSignalSubsystem>()) Signals->Register(this); }
void UCoreKitBasicUIEnemySignalComponent::EndPlay(const EEndPlayReason::Type Reason) { if (auto* Signals = GetWorld()->GetSubsystem<UCoreKitBasicUIWorldSignalSubsystem>()) Signals->Unregister(this); Super::EndPlay(Reason); }
void UCoreKitBasicUIEnemySignalComponent::SetHealth(float Current, float Maximum) {
    if (!FMath::IsFinite(Current) || !FMath::IsFinite(Maximum)) return;
    const float Ratio = Maximum > 0.f ? FMath::Clamp(Current / Maximum, 0.f, 1.f) : 0.f;
    if (Ratio < Bar.Target) { bDamaged = true; LastHitRealTime = FPlatformTime::Seconds(); }
    bAlive = Current > 0.f && Maximum > 0.f;
    Bar.SetTarget(Ratio);
}
void UCoreKitBasicUIEnemySignalComponent::SetCharge(bool bVisible, float Progress) { bChargeVisible = bVisible && FMath::IsFinite(Progress); ChargeProgress = FMath::IsFinite(Progress) ? FMath::Clamp(Progress, 0.f, 1.f) : 0.f; }
float UCoreKitBasicUIEnemySignalComponent::GetSecondsSinceHit() const { return static_cast<float>(FPlatformTime::Seconds() - LastHitRealTime); }
void UCoreKitBasicUIEnemySignalComponent::TickPresentation(float) {
    // Multiple local-player layers may ask to tick; advance once by wall time, never once per player.
    const double Now = FPlatformTime::Seconds();
    if (LastTickRealTime > 0.) Bar.Tick(static_cast<float>(FMath::Clamp(Now - LastTickRealTime, 0., .1)));
    LastTickRealTime = Now;
}
