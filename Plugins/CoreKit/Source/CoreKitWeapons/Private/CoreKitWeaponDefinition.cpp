#include "CoreKitWeapons/CoreKitWeaponDefinition.h"

bool UCoreKitWeaponDefinition::IsUsable() const
{
    const auto Positive = [](float V) { return FMath::IsFinite(V) && V > 0.f; };
    const auto Nonnegative = [](float V) { return FMath::IsFinite(V) && V >= 0.f; };
    if (WeaponId.IsNone() || !Positive(Range) || !Nonnegative(Damage) || !Nonnegative(EquipSeconds))
    {
        return false;
    }
    if (Delivery == ECoreKitWeaponDelivery::Melee)
    {
        if (!Positive(MeleeRadius)) return false;
        for (const FCoreKitWeaponMeleeStep& Step : MeleeSteps)
        {
            if (!Positive(Step.Duration) || !Nonnegative(Step.HitStart) || Step.HitEnd < Step.HitStart ||
                !Nonnegative(Step.HitEnd) || Step.HitEnd > Step.Duration || !Nonnegative(Step.BufferOpen) ||
                !Nonnegative(Step.DamageScale)) return false;
        }
        return true;
    }
    if (!Positive(FireInterval) || !Positive(ReloadSeconds) || !Nonnegative(SpreadDegrees) || SpreadDegrees > 45.f ||
        MagazineSize < 1 || InitialReserve < 0 || AmmoPerShot < 1 || AmmoPerShot > MagazineSize ||
        Pellets < 1 || Pellets > 32 || BurstCount < 1 || BurstCount > 32 ||
        !Nonnegative(BlastDamage) || !Nonnegative(BlastRadius))
    {
        return false;
    }
    return Delivery != ECoreKitWeaponDelivery::Projectile ||
        (Positive(ProjectileSpeed) && Positive(ProjectileLifetime) && Positive(CollisionRadius) &&
         PoolCapacity >= Pellets && PoolCapacity <= 128 && Positive(RicochetRadius));
}
