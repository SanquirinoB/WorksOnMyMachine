#include "CoreKitWeapons/CoreKitWeaponAction.h"

#include "CoreKitWeapons/CoreKitWeaponDefinition.h"
#include "CoreKitWeapons/CoreKitWeaponsComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UCoreKitWeaponsComponent* UCoreKitWeaponAction::FindWeapons() const
{
    const AActor* Owner = GetActionOwner();
    return Owner ? Owner->FindComponentByClass<UCoreKitWeaponsComponent>() : nullptr;
}

bool UCoreKitWeaponAction::EvaluateActivation_Implementation(FName& FailureCode, FString& Detail) const
{
    if (!Super::EvaluateActivation_Implementation(FailureCode, Detail)) return false;
    const auto* Component = FindWeapons();
    if (Component && !Component->ActiveOperation.IsValid() &&
        (GetActionTag() == CoreKitWeaponTags::Reload ? Component->CanReload() : Component->CanFire()))
    {
        return true;
    }
    FailureCode = TEXT("WeaponUnavailable");
    Detail = TEXT("The equipped weapon cannot start this operation.");
    return false;
}

void UCoreKitWeaponAction::Activate()
{
    Super::Activate();
    if (IsEnding()) return;
    Weapons = FindWeapons();
    if (!Weapons.IsValid())
    {
        FailAction();
        return;
    }
    Slot = Weapons->ActiveSlot;
    const auto* WeaponDefinition = Weapons->GetDefinition(Slot);
    if (!WeaponDefinition)
    {
        FailAction();
        return;
    }
    Weapons->ActiveOperation = this;
    if (GetActionTag() == CoreKitWeaponTags::Reload)
    {
        ReloadStarted = GetWorld()->GetTimeSeconds();
        ReloadEnds = ReloadStarted + FMath::Max(.01f, WeaponDefinition->ReloadSeconds);
        if (WeaponDefinition->ReloadSound) UGameplayStatics::PlaySoundAtLocation(Weapons.Get(), WeaponDefinition->ReloadSound,
            Weapons->ResolveAim().Origin);
        Weapons->OnStateChanged.Broadcast();
    }
    else if (WeaponDefinition->Delivery == ECoreKitWeaponDelivery::Melee)
    {
        Weapons->OnFired.Broadcast(Weapons->GetDefinition(Slot));
    }
    else
    {
        BurstRemaining = WeaponDefinition->BurstCount;
        AdvanceFire();
    }
}

void UCoreKitWeaponAction::AdvanceFire()
{
    auto* Component = Weapons.Get();
    if (!Component) { FailAction(); return; }
    const auto& Entry = Component->Slots[Slot];
    const auto& WeaponDefinition = *Entry.Definition;
    if (WeaponDefinition.FireMode == ECoreKitWeaponFireMode::Automatic && !Component->bTriggerHeld)
    {
        CompleteAction();
        return;
    }
    if (GetWorld()->GetTimeSeconds() < Entry.NextShot) return;
    if (!Component->FireRound())
    {
        // A sustained automatic action can wait for pool capacity without losing a round.
        if (WeaponDefinition.FireMode != ECoreKitWeaponFireMode::Automatic || Entry.Magazine < WeaponDefinition.AmmoPerShot)
            FailAction();
        return;
    }
    if (IsEnding()) return;
    --BurstRemaining;
    if (WeaponDefinition.FireMode == ECoreKitWeaponFireMode::Single ||
        (WeaponDefinition.FireMode == ECoreKitWeaponFireMode::Burst && BurstRemaining == 0) ||
        Entry.Magazine < WeaponDefinition.AmmoPerShot)
    {
        CompleteAction();
    }
}

FCoreKitWeaponMeleeStep UCoreKitWeaponAction::GetMeleeStep() const
{
    const auto* WeaponDefinition = Weapons.IsValid() ? Weapons->GetDefinition(Slot) : nullptr;
    return WeaponDefinition && WeaponDefinition->MeleeSteps.IsValidIndex(MeleeStep) ?
        WeaponDefinition->MeleeSteps[MeleeStep] : FCoreKitWeaponMeleeStep();
}

void UCoreKitWeaponAction::AdvanceMelee(float DeltaSeconds)
{
    const FCoreKitWeaponMeleeStep Step = GetMeleeStep();
    const float Previous = MeleeElapsed;
    MeleeElapsed += DeltaSeconds;
    if (MeleeElapsed >= Step.HitStart && Previous <= Step.HitEnd) Weapons->SweepMelee(Step, Victims);
    if (IsEnding() || !Weapons.IsValid() || MeleeElapsed < Step.Duration) return;
    const int32 Next = MeleeStep + 1;
    if (!bMeleeBuffered || !Weapons->GetDefinition(Slot)->MeleeSteps.IsValidIndex(Next))
    {
        CompleteAction();
        return;
    }
    MeleeStep = Next;
    MeleeElapsed = 0.f;
    bMeleeBuffered = false;
    Victims.Reset();
    Weapons->OnFired.Broadcast(Weapons->GetDefinition(Slot));
}

void UCoreKitWeaponAction::TickTask(float DeltaSeconds)
{
    Super::TickTask(DeltaSeconds);
    if (IsEnding()) return;
    auto* Component = Weapons.Get();
    if (!Component || Slot != Component->ActiveSlot) { FailAction(); return; }
    if (!Component->CanOperate()) return;
    const auto* WeaponDefinition = Component->GetDefinition(Slot);
    if (GetActionTag() == CoreKitWeaponTags::Reload)
    {
        if (GetWorld()->GetTimeSeconds() < ReloadEnds) return;
        const int32 Loaded = Component->CommitReload();
        if (Loaded > 0 && WeaponDefinition->ReloadMode == ECoreKitWeaponReloadMode::PerRound && Component->CanReload())
        {
            ReloadStarted = GetWorld()->GetTimeSeconds();
            ReloadEnds = ReloadStarted + FMath::Max(.01f, WeaponDefinition->ReloadSeconds);
            Component->OnStateChanged.Broadcast();
            return;
        }
        if (Loaded > 0 && WeaponDefinition->FireMode == ECoreKitWeaponFireMode::Automatic && Component->bTriggerHeld)
        {
            Component->bPendingResume = true;
            Component->SetComponentTickEnabled(true);
        }
        CompleteAction();
    }
    else if (WeaponDefinition->Delivery == ECoreKitWeaponDelivery::Melee) AdvanceMelee(DeltaSeconds);
    else AdvanceFire();
}

void UCoreKitWeaponAction::CleanupAction(ECoreKitActionEndReason Reason)
{
    if (Weapons.IsValid() && Weapons->ActiveOperation.Get() == this)
    {
        Weapons->ActiveOperation.Reset();
        if (!Weapons->bShuttingDown) Weapons->OnStateChanged.Broadcast();
    }
    Weapons.Reset();
    Victims.Reset();
    Super::CleanupAction(Reason);
}
