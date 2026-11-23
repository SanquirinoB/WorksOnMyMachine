#pragma once

#include "Components/ActorComponent.h"
#include "CoreKitWeapons/CoreKitWeaponTypes.h"
#include "NativeGameplayTags.h"
#include "CoreKitWeaponsComponent.generated.h"

class ACoreKitWeaponProjectile;
class UCoreKitActionComponent;
class UCoreKitWeaponAction;
class UCoreKitWeaponDefinition;

namespace CoreKitWeaponTags
{
    COREKITWEAPONS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fire);
    COREKITWEAPONS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Reload);
    COREKITWEAPONS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equip);
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoreKitWeaponsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitWeaponFired, UCoreKitWeaponDefinition*, Definition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitWeaponContact, const FCoreKitWeaponImpact&, Impact, bool, bAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitWeaponBlast, FVector, Center, float, Radius);

USTRUCT()
struct FCoreKitWeaponSlot
{
    GENERATED_BODY()

    UPROPERTY() TObjectPtr<UCoreKitWeaponDefinition> Definition;
    UPROPERTY() TArray<TObjectPtr<ACoreKitWeaponProjectile>> Pool;
    int32 Magazine = 0;
    int32 Reserve = 0;
    double NextShot = 0.;
};

/** Local weapon runtime. The project owns input, aiming, collision profiles and the impact receiver. */
UCLASS(ClassGroup=(CoreKit), meta=(BlueprintSpawnableComponent))
class COREKITWEAPONS_API UCoreKitWeaponsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCoreKitWeaponsComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapons")
    TArray<TObjectPtr<UCoreKitWeaponDefinition>> InitialLoadout;

    UPROPERTY(BlueprintAssignable, Category="Weapons") FCoreKitWeaponsChanged OnLoadoutChanged;
    UPROPERTY(BlueprintAssignable, Category="Weapons") FCoreKitWeaponsChanged OnEquipped;
    UPROPERTY(BlueprintAssignable, Category="Weapons") FCoreKitWeaponsChanged OnStateChanged;
    UPROPERTY(BlueprintAssignable, Category="Weapons") FCoreKitWeaponFired OnFired;
    UPROPERTY(BlueprintAssignable, Category="Weapons") FCoreKitWeaponContact OnImpact;
    UPROPERTY(BlueprintAssignable, Category="Weapons") FCoreKitWeaponBlast OnBlast;

    UFUNCTION(BlueprintCallable, Category="Weapons") bool GiveWeapon(UCoreKitWeaponDefinition* Definition);
    UFUNCTION(BlueprintCallable, Category="Weapons") bool EquipSlot(int32 Index);
    UFUNCTION(BlueprintCallable, Category="Weapons") bool PressTrigger();
    UFUNCTION(BlueprintCallable, Category="Weapons") void ReleaseTrigger();
    UFUNCTION(BlueprintCallable, Category="Weapons") bool RequestReload();
    UFUNCTION(BlueprintCallable, Category="Weapons") void CancelOperations();
    UFUNCTION(BlueprintCallable, Category="Weapons") void SetAim(const FCoreKitWeaponAim& Aim);

    UFUNCTION(BlueprintPure, Category="Weapons") int32 GetActiveSlot() const { return ActiveSlot; }
    UFUNCTION(BlueprintPure, Category="Weapons") int32 GetSlotCount() const { return Slots.Num(); }
    UFUNCTION(BlueprintPure, Category="Weapons") UCoreKitWeaponDefinition* GetDefinition(int32 Index) const;
    UFUNCTION(BlueprintPure, Category="Weapons") FCoreKitWeaponState GetState() const;

protected:
    void BeginPlay() override;
    void EndPlay(EEndPlayReason::Type Reason) override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

private:
    friend class UCoreKitWeaponAction;
    friend class ACoreKitWeaponProjectile;

    UPROPERTY(Transient) TArray<FCoreKitWeaponSlot> Slots;
    UPROPERTY(Transient) TObjectPtr<UCoreKitActionComponent> Actions;
    TWeakObjectPtr<UCoreKitWeaponAction> ActiveOperation;
    FCoreKitWeaponAim CurrentAim;
    bool bAimSet = false;
    bool bTriggerHeld = false;
    bool bPendingAutoReload = false;
    bool bPendingResume = false;
    bool bDispatchingImpact = false;
    bool bOperating = false;
    bool bShuttingDown = false;
    int32 ActiveSlot = INDEX_NONE;
    double EquipReadyAt = 0.;

    bool CanOperate() const;
    bool CanFire() const;
    bool CanReload() const;
    bool CanEquip(int32 Index) const;
    bool ApplyEquip(int32 Index);
    bool FireRound();
    void SweepMelee(const FCoreKitWeaponMeleeStep& Step, TSet<TWeakObjectPtr<AActor>>& Victims);
    void QueueAutoReload();
    int32 CommitReload();
    FCoreKitWeaponAim ResolveAim() const;
    FCoreKitWeaponImpact MakeImpact(const UCoreKitWeaponDefinition& Definition, const FCoreKitWeaponAim& Aim) const;
    bool ReportImpact(const FCoreKitWeaponImpact& Impact);
    void ResolveBlast(const UCoreKitWeaponDefinition& Definition, FCoreKitWeaponImpact Impact);
    void SetupActions();
};
