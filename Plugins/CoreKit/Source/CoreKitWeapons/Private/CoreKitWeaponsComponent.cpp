#include "CoreKitWeapons/CoreKitWeaponsComponent.h"

#include "CoreKitWeaponProjectile.h"
#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"
#include "CoreKitActions/Actions/CoreKitCommandAction.h"
#include "CoreKitActions/Actions/Data/CoreKitActionDefinition.h"
#include "CoreKitActions/Actions/Data/CoreKitActionSet.h"
#include "CoreKitWeapons/CoreKitWeaponAction.h"
#include "CoreKitWeapons/CoreKitWeaponDefinition.h"
#include "CoreKitWeapons/CoreKitWeaponReceiver.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace CoreKitWeaponTags
{
    UE_DEFINE_GAMEPLAY_TAG(Fire, "CoreKit.Weapons.Action.Fire");
    UE_DEFINE_GAMEPLAY_TAG(Reload, "CoreKit.Weapons.Action.Reload");
    UE_DEFINE_GAMEPLAY_TAG(Equip, "CoreKit.Weapons.Action.Equip");
}

UCoreKitWeaponsComponent::UCoreKitWeaponsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCoreKitWeaponsComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!CanOperate()) return;
    SetupActions();
    for (UCoreKitWeaponDefinition* Definition : InitialLoadout) GiveWeapon(Definition);
}

void UCoreKitWeaponsComponent::SetupActions()
{
    Actions = GetOwner()->FindComponentByClass<UCoreKitActionComponent>();
    const bool bCreate = !Actions;
    if (bCreate)
    {
        Actions = NewObject<UCoreKitActionComponent>(GetOwner());
        GetOwner()->AddInstanceComponent(Actions);
    }
    if (!Actions->ActionSet)
    {
        auto* Set = NewObject<UCoreKitActionSet>(Actions);
        for (const FGameplayTag Tag : {FGameplayTag(CoreKitWeaponTags::Fire), FGameplayTag(CoreKitWeaponTags::Reload), FGameplayTag(CoreKitWeaponTags::Equip)})
        {
            FCoreKitActionEntry& Entry = Set->Actions.Add(Tag);
            const bool bEquip = Tag == CoreKitWeaponTags::Equip;
            Entry.ActionTemplate = bEquip ? static_cast<UCoreKitAction*>(NewObject<UCoreKitCommandAction>(Set)) :
                static_cast<UCoreKitAction*>(NewObject<UCoreKitWeaponAction>(Set));
            Entry.Definition = NewObject<UCoreKitActionDefinition>(Set);
            Entry.Definition->bTickAction = !bEquip;
        }
        Actions->ActionSet = Set;
    }
    if (bCreate) Actions->RegisterComponent();
}

bool UCoreKitWeaponsComponent::CanOperate() const
{
    return !bShuttingDown && !bDispatchingImpact && !bOperating && IsValid(GetOwner()) &&
        GetOwner()->HasAuthority() && GetWorld() && !GetWorld()->IsPaused();
}

bool UCoreKitWeaponsComponent::GiveWeapon(UCoreKitWeaponDefinition* Definition)
{
    if (!CanOperate() || !IsValid(Definition) || !Definition->IsUsable() || Slots.Num() >= 32 ||
        Slots.ContainsByPredicate([Definition](const FCoreKitWeaponSlot& Entry)
        {
            return Entry.Definition->WeaponId == Definition->WeaponId;
        }))
    {
        return false;
    }
    const int32 Index = Slots.AddDefaulted();
    auto& Entry = Slots[Index];
    Entry.Definition = Definition;
    if (Definition->Delivery != ECoreKitWeaponDelivery::Melee)
    {
        Entry.Magazine = Definition->MagazineSize;
        Entry.Reserve = Definition->InitialReserve;
    }
    const bool bFirst = ActiveSlot == INDEX_NONE;
    {
        TGuardValue<bool> Guard(bOperating, true);
        OnLoadoutChanged.Broadcast();
    }
    if (bFirst && CanOperate()) EquipSlot(Index);
    return true;
}

UCoreKitWeaponDefinition* UCoreKitWeaponsComponent::GetDefinition(int32 Index) const
{
    return Slots.IsValidIndex(Index) ? Slots[Index].Definition.Get() : nullptr;
}

bool UCoreKitWeaponsComponent::CanEquip(int32 Index) const
{
    return CanOperate() && Slots.IsValidIndex(Index) && ActiveSlot != Index;
}

bool UCoreKitWeaponsComponent::EquipSlot(int32 Index)
{
    if (!Actions || !CanEquip(Index)) return false;
    // Consult policy before stopping old work. Never cancel a rejected switch.
    if (Actions->EvaluateCommandPolicy(CoreKitWeaponTags::Equip, this).Status != ECoreKitActionRequestStatus::Allowed) return false;
    CancelOperations();
    const auto Report = Actions->ExecuteActionCommand(CoreKitWeaponTags::Equip, FGameplayTag(), this,
        [this, Index](FName&, FString&) { return CanEquip(Index); },
        [this, Index](FName&, FString&) { return ApplyEquip(Index); });
    return Report.Status == ECoreKitActionRequestStatus::Executed;
}

bool UCoreKitWeaponsComponent::ApplyEquip(int32 Index)
{
    if (ActiveOperation.IsValid() || !CanEquip(Index)) return false;
    ActiveSlot = Index;
    EquipReadyAt = GetWorld()->GetTimeSeconds() + Slots[Index].Definition->EquipSeconds;
    TGuardValue<bool> Guard(bOperating, true);
    OnEquipped.Broadcast();
    OnStateChanged.Broadcast();
    return true;
}

FCoreKitWeaponState UCoreKitWeaponsComponent::GetState() const
{
    FCoreKitWeaponState State;
    if (!Slots.IsValidIndex(ActiveSlot)) return State;
    const auto& Entry = Slots[ActiveSlot];
    State.Magazine = Entry.Magazine;
    State.Reserve = Entry.Reserve;
    State.Capacity = Entry.Definition->Delivery == ECoreKitWeaponDelivery::Melee ? 0 : Entry.Definition->MagazineSize;
    State.bUnlimitedReserve = Entry.Definition->bUnlimitedReserve;
    if (const auto* Operation = ActiveOperation.Get(); Operation && Operation->GetActionTag() == CoreKitWeaponTags::Reload)
    {
        State.bReloading = true;
        State.ReloadProgress = FMath::Clamp(static_cast<float>(
            (GetWorld()->GetTimeSeconds() - Operation->ReloadStarted) / FMath::Max(.01, Operation->ReloadEnds - Operation->ReloadStarted)), 0.f, 1.f);
    }
    return State;
}

bool UCoreKitWeaponsComponent::CanFire() const
{
    if (!CanOperate() || !Slots.IsValidIndex(ActiveSlot)) return false;
    const auto& Entry = Slots[ActiveSlot];
    return GetWorld()->GetTimeSeconds() >= FMath::Max(EquipReadyAt, Entry.NextShot) &&
        (Entry.Definition->Delivery == ECoreKitWeaponDelivery::Melee || Entry.Magazine >= Entry.Definition->AmmoPerShot);
}

bool UCoreKitWeaponsComponent::CanReload() const
{
    if (!CanOperate() || !Slots.IsValidIndex(ActiveSlot) || GetWorld()->GetTimeSeconds() < EquipReadyAt) return false;
    const auto& Entry = Slots[ActiveSlot];
    return Entry.Definition->Delivery != ECoreKitWeaponDelivery::Melee &&
        Entry.Definition->ReloadMode != ECoreKitWeaponReloadMode::Disabled &&
        Entry.Magazine < Entry.Definition->MagazineSize &&
        (Entry.Reserve > 0 || Entry.Definition->bUnlimitedReserve);
}

bool UCoreKitWeaponsComponent::PressTrigger()
{
    UCoreKitWeaponDefinition* Definition = GetDefinition(ActiveSlot);
    if (!CanOperate() || !Actions || !Definition) return false;
    bTriggerHeld = true;
    if (auto* Active = ActiveOperation.Get())
    {
        if (Active->GetActionTag() == CoreKitWeaponTags::Reload &&
            Definition->ReloadMode == ECoreKitWeaponReloadMode::PerRound && CanFire())
        {
            Actions->CancelExecution(Active->GetExecutionId());
        }
        else
        {
            if (Definition->Delivery == ECoreKitWeaponDelivery::Melee &&
                Active->GetActionTag() == CoreKitWeaponTags::Fire &&
                Active->MeleeElapsed >= Active->GetMeleeStep().BufferOpen)
            {
                Active->bMeleeBuffered = true;
                return true;
            }
            return false;
        }
    }
    const auto Report = Actions->RequestActionWithContext(CoreKitWeaponTags::Fire, FGameplayTag(), nullptr);
    if (Report.Status == ECoreKitActionRequestStatus::Queued) Actions->CancelQueuedRequest(Report.RequestId);
    if (Report.Status == ECoreKitActionRequestStatus::Started) return true;
    if (Slots[ActiveSlot].Magazine < Definition->AmmoPerShot && Definition->bAutoReload) QueueAutoReload();
    return false;
}

void UCoreKitWeaponsComponent::ReleaseTrigger()
{
    bTriggerHeld = false;
    bPendingResume = false;
    if (auto* Active = ActiveOperation.Get(); Active && Active->GetActionTag() == CoreKitWeaponTags::Fire &&
        GetDefinition(ActiveSlot) && GetDefinition(ActiveSlot)->FireMode == ECoreKitWeaponFireMode::Automatic && Actions)
    {
        Actions->CancelExecution(Active->GetExecutionId());
    }
}

bool UCoreKitWeaponsComponent::RequestReload()
{
    if (!Actions || !CanReload()) return false;
    if (auto* Active = ActiveOperation.Get())
    {
        if (Active->GetActionTag() == CoreKitWeaponTags::Reload) return false;
        if (!Actions->CancelExecution(Active->GetExecutionId())) return false;
    }
    const auto Report = Actions->RequestActionWithContext(CoreKitWeaponTags::Reload, FGameplayTag(), nullptr);
    if (Report.Status == ECoreKitActionRequestStatus::Queued) Actions->CancelQueuedRequest(Report.RequestId);
    return Report.Status == ECoreKitActionRequestStatus::Started;
}

void UCoreKitWeaponsComponent::QueueAutoReload()
{
    bPendingAutoReload = true;
    SetComponentTickEnabled(true);
}

void UCoreKitWeaponsComponent::CancelOperations()
{
    bTriggerHeld = bPendingAutoReload = bPendingResume = false;
    SetComponentTickEnabled(false);
    if (auto* Active = ActiveOperation.Get(); Active && Actions) Actions->CancelExecution(Active->GetExecutionId());
}

void UCoreKitWeaponsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (!CanOperate() || ActiveOperation.IsValid()) return;
    if (bPendingAutoReload)
    {
        bPendingAutoReload = false;
        RequestReload();
    }
    if (bPendingResume && !ActiveOperation.IsValid())
    {
        if (bTriggerHeld && GetDefinition(ActiveSlot) && GetWorld()->GetTimeSeconds() < Slots[ActiveSlot].NextShot) return;
        bPendingResume = false;
        if (bTriggerHeld) PressTrigger();
    }
    SetComponentTickEnabled(bPendingAutoReload || bPendingResume);
}

int32 UCoreKitWeaponsComponent::CommitReload()
{
    auto& Entry = Slots[ActiveSlot];
    const auto& Definition = *Entry.Definition;
    const int32 Capacity = Definition.MagazineSize - Entry.Magazine;
    const int32 Available = Definition.bUnlimitedReserve ? Capacity : FMath::Min(Capacity, Entry.Reserve);
    const int32 Count = Definition.ReloadMode == ECoreKitWeaponReloadMode::PerRound ? FMath::Min(1, Available) : Available;
    Entry.Magazine += Count;
    if (!Definition.bUnlimitedReserve) Entry.Reserve -= Count;
    return Count;
}

void UCoreKitWeaponsComponent::SetAim(const FCoreKitWeaponAim& Aim)
{
    if (Aim.Origin.ContainsNaN() || Aim.Direction.ContainsNaN() || Aim.PlaneNormal.ContainsNaN()) return;
    FCoreKitWeaponAim Normalized = Aim;
    Normalized.PlaneNormal = Aim.PlaneNormal.GetSafeNormal();
    Normalized.Direction = FVector::VectorPlaneProject(Aim.Direction, Normalized.PlaneNormal).GetSafeNormal();
    if (Normalized.Direction.IsNearlyZero()) return;
    CurrentAim = Normalized;
    bAimSet = true;
}

FCoreKitWeaponAim UCoreKitWeaponsComponent::ResolveAim() const
{
    if (bAimSet) return CurrentAim;
    FCoreKitWeaponAim Aim;
    Aim.Direction = GetOwner()->GetActorForwardVector();
    Aim.Origin = GetOwner()->GetActorLocation() + Aim.Direction * 40.;
    return Aim;
}

FCoreKitWeaponImpact UCoreKitWeaponsComponent::MakeImpact(const UCoreKitWeaponDefinition& Definition, const FCoreKitWeaponAim& Aim) const
{
    FCoreKitWeaponImpact Impact;
    Impact.InstigatorActor = GetOwner();
    Impact.WeaponId = Definition.WeaponId;
    Impact.DamageTags = Definition.DamageTags;
    Impact.Damage = Definition.Damage;
    Impact.Origin = Aim.Origin;
    Impact.Direction = Aim.Direction;
    return Impact;
}

bool UCoreKitWeaponsComponent::FireRound()
{
    if (!CanFire()) return false;
    auto& Slot = Slots[ActiveSlot];
    auto& Definition = *Slot.Definition;
    TArray<ACoreKitWeaponProjectile*, TInlineAllocator<32>> Free;
    if (Definition.Delivery == ECoreKitWeaponDelivery::Projectile)
    {
        Slot.Pool.RemoveAll([](const auto& P) { return !IsValid(P); });
        for (ACoreKitWeaponProjectile* Projectile : Slot.Pool) if (!Projectile->IsLeased()) Free.Add(Projectile);
        while (Free.Num() < Definition.Pellets && Slot.Pool.Num() < Definition.PoolCapacity)
        {
            auto* Projectile = GetWorld()->SpawnActor<ACoreKitWeaponProjectile>();
            if (!Projectile) return false;
            Slot.Pool.Add(Projectile);
            Free.Add(Projectile);
        }
        if (Free.Num() < Definition.Pellets) return false;
    }
    TGuardValue<bool> Guard(bOperating, true);
    const FCoreKitWeaponAim Aim = ResolveAim();
    const bool bEmptyAfter = Slot.Magazine - Definition.AmmoPerShot < Definition.AmmoPerShot;
    Slot.Magazine -= Definition.AmmoPerShot;
    Slot.NextShot = GetWorld()->GetTimeSeconds() + FMath::Max(.01f, Definition.FireInterval);
    for (int32 Index = 0; Index < Definition.Pellets; ++Index)
    {
        FCoreKitWeaponAim Shot = Aim;
        if (Definition.SpreadDegrees > 0.f)
        {
            Shot.Direction = Aim.PlaneNormal.IsNearlyZero() ?
                FMath::VRandCone(Aim.Direction, FMath::DegreesToRadians(Definition.SpreadDegrees)) :
                Aim.Direction.RotateAngleAxis(FMath::FRandRange(-Definition.SpreadDegrees, Definition.SpreadDegrees), Aim.PlaneNormal);
        }
        FCoreKitWeaponImpact Impact = MakeImpact(Definition, Shot);
        if (Definition.Delivery == ECoreKitWeaponDelivery::Projectile)
        {
            Free[Index]->Launch(*this, Definition, Shot, Impact);
        }
        else
        {
            FCollisionQueryParams Params(SCENE_QUERY_STAT(CoreKitWeaponTrace), false, GetOwner());
            if (GetWorld()->LineTraceSingleByChannel(Impact.Hit, Shot.Origin, Shot.Origin + Shot.Direction * Definition.Range,
                Definition.TraceChannel, Params))
            {
                ReportImpact(Impact);
                ResolveBlast(Definition, Impact);
            }
        }
        if (bShuttingDown) return true;
    }
    if (Definition.FireSound) UGameplayStatics::PlaySoundAtLocation(this, Definition.FireSound, Aim.Origin);
    OnStateChanged.Broadcast();
    OnFired.Broadcast(&Definition);
    if (bEmptyAfter && Definition.bAutoReload && !bShuttingDown) QueueAutoReload();
    return true;
}

void UCoreKitWeaponsComponent::SweepMelee(const FCoreKitWeaponMeleeStep& Step, TSet<TWeakObjectPtr<AActor>>& Victims)
{
    const UCoreKitWeaponDefinition* Definition = GetDefinition(ActiveSlot);
    if (!Definition) return;
    const FCoreKitWeaponAim Aim = ResolveAim();
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoreKitWeaponMelee), false, GetOwner());
    for (const auto& Victim : Victims) if (Victim.IsValid()) Params.AddIgnoredActor(Victim.Get());
    GetWorld()->SweepMultiByChannel(Hits, Aim.Origin, Aim.Origin + Aim.Direction * Definition->Range,
        FQuat::Identity, Definition->TraceChannel, FCollisionShape::MakeSphere(Definition->MeleeRadius), Params);
    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!IsValid(Target) || Victims.Contains(Target)) continue;
        FCoreKitWeaponImpact Impact = MakeImpact(*Definition, Aim);
        Impact.Hit = Hit;
        Impact.Damage *= Step.DamageScale;
        if (ReportImpact(Impact)) Victims.Add(Target);
        if (bShuttingDown) return;
    }
}

bool UCoreKitWeaponsComponent::ReportImpact(const FCoreKitWeaponImpact& Impact)
{
    if (bShuttingDown || bDispatchingImpact) return false;
    TGuardValue<bool> Guard(bDispatchingImpact, true);
    AActor* Target = Impact.Hit.GetActor();
    const bool bAccepted = Impact.Damage > 0.f && IsValid(Target) &&
        Target->Implements<UCoreKitWeaponReceiver>() && ICoreKitWeaponReceiver::Execute_ReceiveWeaponImpact(Target, Impact);
    if (!bShuttingDown) OnImpact.Broadcast(Impact, bAccepted);
    return bAccepted;
}

void UCoreKitWeaponsComponent::ResolveBlast(const UCoreKitWeaponDefinition& Definition, FCoreKitWeaponImpact Impact)
{
    if (bShuttingDown) return;
    if (Definition.ImpactSound) UGameplayStatics::PlaySoundAtLocation(this, Definition.ImpactSound, Impact.Hit.ImpactPoint);
    if (Definition.BlastRadius <= 0.f || Definition.BlastDamage <= 0.f) return;
    TGuardValue<bool> OperationGuard(bOperating, true);
    const FVector Center = Impact.Hit.ImpactPoint + Impact.Hit.ImpactNormal * 2.;
    AActor* Direct = Impact.Hit.GetActor();
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoreKitWeaponBlast), false, Impact.InstigatorActor);
    GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
        FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeSphere(Definition.BlastRadius), Params);
    TSet<TWeakObjectPtr<AActor>> Seen;
    Impact.Kind = ECoreKitWeaponImpactKind::Area;
    Impact.Origin = Center;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Target = Overlap.GetActor();
        if (!IsValid(Target) || (Target == Direct && Definition.Damage > 0.f) ||
            Seen.Contains(Target) || !Target->Implements<UCoreKitWeaponReceiver>()) continue;
        const FVector TargetPoint = Overlap.GetComponent() ? Overlap.GetComponent()->Bounds.Origin : Target->GetActorLocation();
        const float Alpha = 1.f - FMath::Clamp(static_cast<float>(FVector::Distance(Center, TargetPoint) / Definition.BlastRadius), 0.f, 1.f);
        FHitResult Cover;
        if (Alpha <= 0.f || (GetWorld()->LineTraceSingleByChannel(Cover, Center, TargetPoint, Definition.TraceChannel, Params) &&
            Cover.GetActor() != Target)) continue;
        Seen.Add(Target);
        Impact.Hit = FHitResult(Target, Overlap.GetComponent(), TargetPoint, FVector::ZeroVector);
        Impact.Direction = (TargetPoint - Center).GetSafeNormal();
        Impact.Damage = Definition.BlastDamage * Alpha;
        ReportImpact(Impact);
        if (bShuttingDown) return;
    }
    OnBlast.Broadcast(Center, Definition.BlastRadius);
}

void UCoreKitWeaponsComponent::EndPlay(EEndPlayReason::Type Reason)
{
    bShuttingDown = true;
    CancelOperations();
    for (auto& Slot : Slots)
    {
        for (ACoreKitWeaponProjectile* Projectile : Slot.Pool) if (IsValid(Projectile)) Projectile->Destroy();
        Slot.Pool.Reset();
    }
    Super::EndPlay(Reason);
}
