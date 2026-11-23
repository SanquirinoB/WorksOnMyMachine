#include "Misc/AutomationTest.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitBasicUI/Events/CoreKitBasicUIEvents.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextTypes.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIArenaMinimapDefinition.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasicUIIsolationTest,"CoreKit.BasicUI.Consumer.LocalPlayerIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBasicUIIsolationTest::RunTest(const FString&) {
    auto* A=UCoreKitBasicUIContextSubsystem::CreateDetachedForTests(GetTransientPackage());
    auto* B=UCoreKitBasicUIContextSubsystem::CreateDetachedForTests(GetTransientPackage());
    A->GetVitals()->SetHealth(27.f); B->GetVitals()->SetHealth(89.f);
    TestEqual(TEXT("Player A independent"),A->GetVitals()->GetHealth(),27.f);
    TestEqual(TEXT("Player B independent"),B->GetVitals()->GetHealth(),89.f);
    TestFalse(TEXT("Unconfigured card choice is rejected"),A->GetProgression()->RequestChooseCard(0));
    int32 Calls=0; A->GetRun()->Restart.BindLambda([&Calls]{++Calls;});
    A->GetRun()->RequestRestart();TestEqual(TEXT("Command reaches consumer once"),Calls,1);
    A->GetRun()->Restart.Unbind(); A->GetRun()->RequestRestart();TestEqual(TEXT("Unbound command has no side effects"),Calls,1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasicUIEventsTest,"CoreKit.BasicUI.Consumer.EventLifetime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBasicUIEventsTest::RunTest(const FString&) {
    auto* Player=NewObject<ULocalPlayer>(GEngine); auto* Events=NewObject<UCoreKitBasicUIEventChannel>(Player);
    int32 Calls=0;
    auto Handle=Events->Subscribe(CoreKitBasicUIEvent::DamageTaken,FCoreKitBasicUIEventDelegate::FDelegate::CreateLambda([&Calls](FGameplayTag,const FInstancedStruct& Payload){if(Payload.GetPtr<FCoreKitBasicUIDamageTakenEvent>()) ++Calls;}));
    FCoreKitBasicUIDamageTakenEvent Damage; Damage.Amount=12.f;
    Events->Publish(CoreKitBasicUIEvent::DamageTaken,Damage);
    TestEqual(TEXT("Typed payload delivered"),Calls,1);
    Events->Unsubscribe(CoreKitBasicUIEvent::DamageTaken,Handle); Events->Publish(CoreKitBasicUIEvent::DamageTaken,Damage);
    TestEqual(TEXT("Removed subscription stays silent"),Calls,1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasicUIAssetsTest,"CoreKit.BasicUI.Consumer.PortableContent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBasicUIAssetsTest::RunTest(const FString&) {
    auto* Numbers=LoadObject<UCoreKitBasicUIFloatingTextDefinition>(nullptr,TEXT("/CoreKit/BasicUI/FloatingText/DA_DamageNumbers.DA_DamageNumbers"));
    TestNotNull(TEXT("Own floating-text schema loads without CombatKit"),Numbers);
    if(Numbers) { TestTrue(TEXT("Migrated styles usable"),Numbers->IsUsable());TestNotNull(TEXT("Rifle preset"),Numbers->FindStyle(TEXT("DDRifle")));TestNotNull(TEXT("Electric preset"),Numbers->FindStyle(TEXT("DDZap"))); }
    auto* Plan=LoadObject<UCoreKitBasicUIArenaMinimapDefinition>(nullptr,TEXT("/CoreKit/BasicUI/Minimap/DA_Minimap_Example.DA_Minimap_Example"));
    TestNotNull(TEXT("Own minimap schema"),Plan); if(Plan)TestTrue(TEXT("Example does not pull DeadDraw map"),Plan->Map.IsNull());
    return true;
}
#endif
