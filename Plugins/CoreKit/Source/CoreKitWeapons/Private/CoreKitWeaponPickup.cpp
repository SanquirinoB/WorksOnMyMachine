#include "CoreKitWeapons/CoreKitWeaponPickup.h"

#include "Components/StaticMeshComponent.h"
#include "CoreKitWeapons/CoreKitWeaponsComponent.h"

ACoreKitWeaponPickup::ACoreKitWeaponPickup()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
}

bool ACoreKitWeaponPickup::TryCollect(AActor* Receiver)
{
    if (!HasAuthority() || bCollecting || !IsValid(Receiver) ||
        FVector::DistSquared(Receiver->GetActorLocation(), GetActorLocation()) > FMath::Square(InteractionRange))
    {
        return false;
    }
    auto* Equipment = Receiver->FindComponentByClass<UCoreKitWeaponsComponent>();
    if (!Equipment) return false;
    TGuardValue<bool> Guard(bCollecting, true);
    if (!Equipment->GiveWeapon(Definition)) return false;
    Destroy();
    return true;
}
