#if WITH_DEV_AUTOMATION_TESTS

#include "../DMoverComponent.h"
#include "../DPawn.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    class FDReengageMagnet final : public IAutomationLatentCommand
    {
    public:
        explicit FDReengageMagnet(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            if (FPlatformTime::Seconds() - Started > 10.)
            {
                Test.AddError(FString::Printf(TEXT("Magnetic walking stalled at step %d."), static_cast<int32>(Step)));
                return true;
            }

            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            const auto* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            const auto* Mover = Pawn ? Pawn->FindComponentByClass<UDMoverComponent>() : nullptr;
            if (!Mover || World->GetTimeSeconds() < 0.3)
            {
                return false;
            }
            const auto SetKey = [Player](const FKey& Key, bool bHeld)
            {
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(Key, bHeld ? IE_Pressed : IE_Released, bHeld ? 1.f : 0.f));
            };

            switch (Step)
            {
            case EStep::Start:
                SetKey(EKeys::E, true);
                Step = EStep::Attach;
                break;
            case EStep::Attach:
                if (Mover->IsMagneticallyAttached())
                {
                    SetKey(EKeys::E, false);
                    Step = EStep::Release;
                }
                break;
            case EStep::Release:
                if (Mover->IsFlying())
                {
                    StartX = Pawn->GetActorLocation().X;
                    SetKey(EKeys::E, true);
                    SetKey(EKeys::D, true);
                    CheckTime = World->GetTimeSeconds() + 0.4;
                    Step = EStep::Walk;
                }
                break;
            case EStep::Walk:
                if (World->GetTimeSeconds() >= CheckTime)
                {
                    Test.TestTrue(TEXT("Holding E again reattaches the player to the same deck"), Mover->IsMagneticallyAttached());
                    Test.TestTrue(TEXT("Holding D with E walks right across the opening deck"), Pawn->GetActorLocation().X > StartX + 35.);
                    SetKey(EKeys::D, false);
                    SetKey(EKeys::E, false);
                    return true;
                }
                break;
            }
            return false;
        }

    private:
        enum class EStep : uint8 { Start, Attach, Release, Walk };

        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        EStep Step = EStep::Start;
        double StartX = 0.;
        double CheckTime = 0.;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMagneticReengageTest, "Delerict.Player.MagneticReengage",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDMagneticReengageTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DOpening"), true))
    {
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDReengageMagnet(*this));
    return true;
}

#endif