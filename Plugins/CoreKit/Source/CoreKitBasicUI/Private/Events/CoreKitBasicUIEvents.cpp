#include "CoreKitBasicUI/Events/CoreKitBasicUIEvents.h"

namespace CoreKitBasicUIEvent
{
	UE_DEFINE_GAMEPLAY_TAG(Hit, "CoreKit.BasicUI.Event.Hit");
	UE_DEFINE_GAMEPLAY_TAG(DamageTaken, "CoreKit.BasicUI.Event.DamageTaken");
	UE_DEFINE_GAMEPLAY_TAG(Healed, "CoreKit.BasicUI.Event.Healed");
	UE_DEFINE_GAMEPLAY_TAG(Pickup, "CoreKit.BasicUI.Event.Pickup");
	UE_DEFINE_GAMEPLAY_TAG(Dashed, "CoreKit.BasicUI.Event.Dashed");
	UE_DEFINE_GAMEPLAY_TAG(DashReady, "CoreKit.BasicUI.Event.DashReady");
	UE_DEFINE_GAMEPLAY_TAG(WeaponSwapped, "CoreKit.BasicUI.Event.WeaponSwapped");
	UE_DEFINE_GAMEPLAY_TAG(ReloadStarted, "CoreKit.BasicUI.Event.ReloadStarted");
	UE_DEFINE_GAMEPLAY_TAG(ReloadCompleted, "CoreKit.BasicUI.Event.ReloadCompleted");
	UE_DEFINE_GAMEPLAY_TAG(LevelUp, "CoreKit.BasicUI.Event.LevelUp");
	UE_DEFINE_GAMEPLAY_TAG(CardChosen, "CoreKit.BasicUI.Event.CardChosen");
	UE_DEFINE_GAMEPLAY_TAG(StageChanged, "CoreKit.BasicUI.Event.StageChanged");
	UE_DEFINE_GAMEPLAY_TAG(BossInbound, "CoreKit.BasicUI.Event.BossInbound");
	UE_DEFINE_GAMEPLAY_TAG(BossSpawned, "CoreKit.BasicUI.Event.BossSpawned");
	UE_DEFINE_GAMEPLAY_TAG(BossTelegraph, "CoreKit.BasicUI.Event.BossTelegraph");
	UE_DEFINE_GAMEPLAY_TAG(BossDefeated, "CoreKit.BasicUI.Event.BossDefeated");
	UE_DEFINE_GAMEPLAY_TAG(ThreatLock, "CoreKit.BasicUI.Event.ThreatLock");
	UE_DEFINE_GAMEPLAY_TAG(RunEnded, "CoreKit.BasicUI.Event.RunEnded");
}

void UCoreKitBasicUIEventChannel::Publish(FGameplayTag Event, const FInstancedStruct& Payload)
{
	if (FCoreKitBasicUIEventDelegate* Delegate = Listeners.Find(Event))
	{
		Delegate->Broadcast(Event, Payload);
	}
	OnAnyEvent.Broadcast(Event, Payload);
}

FDelegateHandle UCoreKitBasicUIEventChannel::Subscribe(FGameplayTag Event, FCoreKitBasicUIEventDelegate::FDelegate Delegate)
{
	return Listeners.FindOrAdd(Event).Add(MoveTemp(Delegate));
}

void UCoreKitBasicUIEventChannel::Unsubscribe(FGameplayTag Event, FDelegateHandle Handle)
{
	if (FCoreKitBasicUIEventDelegate* Delegate = Listeners.Find(Event))
	{
		Delegate->Remove(Handle);
	}
}
