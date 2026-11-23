#if DELERICT_WITH_GAMEPLAY_DEBUG && WITH_DEV_AUTOMATION_TESTS

#include "../DGameplayDebug.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/StringOutputDevice.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDGameplayDebugChannelsTest, "Delerict.Debug.WorldChannels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDGameplayDebugChannelsTest::RunTest(const FString& Parameters)
{
    using DGameplayDebug::EChannel;
    using DGameplayDebug::IsEnabled;

    UWorld* FirstWorld = UWorld::CreateWorld(EWorldType::Game, false);
    UWorld* SecondWorld = UWorld::CreateWorld(EWorldType::Game, false);
    const auto Run = [this](UWorld* World, const TCHAR* Command)
    {
        FStringOutputDevice Output;
        Output.SetAutoEmitLineTerminator(true);
        TestTrue(Command, IConsoleManager::Get().ProcessUserConsoleInput(Command, Output, World));
        return FString(Output);
    };
    const auto List = [&Run](UWorld* World) { return Run(World, TEXT("Delerict.Debug List")); };

    const FString Initial = List(FirstWorld);
    TestTrue(TEXT("All channels start off"), Initial.Contains(TEXT("Delerict.Debug active: None")));
    TestTrue(TEXT("List shows all available channels"),
        Initial.Contains(TEXT("Delerict.Debug available: Tuning, Flight, Aim, Collision, Throws, Weapons, Magnet, Jump")));

    Run(FirstWorld, TEXT("Delerict.Debug On Flight Aim"));
    TestTrue(TEXT("List shows both enabled channels"), List(FirstWorld).Contains(TEXT("Delerict.Debug active: Flight, Aim")));
    TestTrue(TEXT("The first world has both channels enabled"),
        IsEnabled(FirstWorld, EChannel::Flight) && IsEnabled(FirstWorld, EChannel::Aim));
    TestTrue(TEXT("The second world starts off"), List(SecondWorld).Contains(TEXT("Delerict.Debug active: None")));

    Run(FirstWorld, TEXT("Delerict.Debug Off Aim"));
    TestTrue(TEXT("Disabling Aim keeps Flight enabled"),
        IsEnabled(FirstWorld, EChannel::Flight) && !IsEnabled(FirstWorld, EChannel::Aim));
    TestTrue(TEXT("List shows Flight alone"),
        List(FirstWorld).Contains(TEXT("Delerict.Debug active: Flight") LINE_TERMINATOR));

    Run(FirstWorld, TEXT("Delerict.Debug On Aim Unknown"));
    TestFalse(TEXT("An unknown channel rejects the whole change"), IsEnabled(FirstWorld, EChannel::Aim));

    Run(SecondWorld, TEXT("Delerict.Debug On All"));
    TestTrue(TEXT("On All enables every channel in the second world"),
        List(SecondWorld).Contains(TEXT("Delerict.Debug active: Tuning, Flight, Aim, Collision, Throws, Weapons, Magnet, Jump")));
    Run(FirstWorld, TEXT("Delerict.Debug Off"));
    TestTrue(TEXT("Off clears all channels in the first world"),
        List(FirstWorld).Contains(TEXT("Delerict.Debug active: None")));
    TestTrue(TEXT("The second world keeps its channels"), IsEnabled(SecondWorld, EChannel::Aim));

    FirstWorld->DestroyWorld(false);
    SecondWorld->DestroyWorld(false);
    return true;
}

#endif
