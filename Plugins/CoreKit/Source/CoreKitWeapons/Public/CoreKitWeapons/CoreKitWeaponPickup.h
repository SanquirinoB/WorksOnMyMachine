#pragma once

#include "GameFramework/Actor.h"
#include "CoreKitWeaponPickup.generated.h"

class UCoreKitWeaponDefinition;
class UStaticMeshComponent;

/** Call TryCollect from the game's interaction system. A rejected pickup stays available. */
UCLASS(Blueprintable)
class COREKITWEAPONS_API ACoreKitWeaponPickup : public AActor
{
    GENERATED_BODY()

public:
    ACoreKitWeaponPickup();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapons") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapons") TObjectPtr<UCoreKitWeaponDefinition> Definition;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapons", meta=(ClampMin="0")) float InteractionRange = 180.f;
    UFUNCTION(BlueprintCallable, Category="Weapons") bool TryCollect(AActor* Receiver);

private:
    bool bCollecting = false;
};
