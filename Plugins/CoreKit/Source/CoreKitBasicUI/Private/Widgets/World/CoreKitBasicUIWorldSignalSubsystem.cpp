#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalSubsystem.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalLayer.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIEnemySignalComponent.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
bool UCoreKitBasicUIWorldSignalSubsystem::IsEnabled() { return GetDefault<UCoreKitBasicUISettings>()->bEnableWorldSignals; }
bool UCoreKitBasicUIWorldSignalSubsystem::ShouldCreateSubsystem(UObject* Outer) const { const UWorld* World = Cast<UWorld>(Outer); return IsEnabled() && World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer); }
void UCoreKitBasicUIWorldSignalSubsystem::Deinitialize() { for (auto& Pair : Layers) if (Pair.Value) Pair.Value->RemoveFromParent(); Layers.Reset(); Signals.Reset(); Super::Deinitialize(); }
void UCoreKitBasicUIWorldSignalSubsystem::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    UWorld* World = GetWorld();
    UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
    if (!Game || World->GetNetMode() == NM_DedicatedServer) return;
    for (auto It = Layers.CreateIterator(); It; ++It) { if (!Game->GetLocalPlayers().Contains(It.Key())) { if (It.Value()) It.Value()->RemoveFromParent(); It.RemoveCurrent(); } }
    for (ULocalPlayer* Player : Game->GetLocalPlayers()) {
        APlayerController* PC = Player ? Player->GetPlayerController(World) : nullptr;
        if (PC && Player->ViewportClient && !Layers.Contains(Player)) {
            auto* Layer = CreateWidget<UCoreKitBasicUIWorldSignalLayer>(PC, UCoreKitBasicUIWorldSignalLayer::StaticClass());
            if (Layer) { Layer->AddToPlayerScreen(-10); Layers.Add(Player, Layer); }
        }
    }
    Signals.RemoveAll([](const auto& Signal){ return !Signal.IsValid(); });
}
void UCoreKitBasicUIWorldSignalSubsystem::Register(UCoreKitBasicUIEnemySignalComponent* Signal) { Signals.AddUnique(Signal); }
void UCoreKitBasicUIWorldSignalSubsystem::Unregister(UCoreKitBasicUIEnemySignalComponent* Signal) { Signals.Remove(Signal); }
