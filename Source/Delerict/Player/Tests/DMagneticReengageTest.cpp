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
            if (!Mover || World->GetTimeSeconds() < CheckTime)
            {
                return false;
            }
            const auto SetKey = [Player](const FKey& Key, bool bHeld)
            {
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(Key, bHeld ? IE_Pressed : IE_Released, bHeld ? 1.f : 0.f));
            };
            const auto Advance = [this, World](EStep Next, double Delay = 0.)
            {
                Step = Next;
                CheckTime = World->GetTimeSeconds() + Delay;
            };

            switch (Step)
            {
            case EStep::Start:
                Test.TestTrue(TEXT("The opening deck requires E to capture a floating player"), Mover->IsFlying());
                SetKey(EKeys::E, true);
                Advance(EStep::Attach);
                break;

            case EStep::Attach:
                if (Mover->IsMagneticallyAttached())
                {
                    StartX = Pawn->GetActorLocation().X;
                    SetKey(EKeys::E, false);
                    SetKey(EKeys::D, true);
                    Advance(EStep::Walk, 0.4);
                }
                break;

            case EStep::Walk:
                Test.TestTrue(TEXT("Releasing E keeps the pawn attached"), Mover->IsMagneticallyAttached());
                Test.TestTrue(TEXT("D walks right without E held"), Pawn->GetActorLocation().X > StartX + 35.);
                SetKey(EKeys::D, false);
                SetKey(EKeys::E, true);
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightX, IE_Axis, 1.f));
                Advance(EStep::ChargeThrow, 0.2);
                break;

            case EStep::ChargeThrow:
                Test.TestTrue(TEXT("Pressing E again does not release attached boots"), Mover->IsMagneticallyAttached());
                PlateCount = Pawn->GetPlateCount();
                SetKey(EKeys::RightMouseButton, true);
                Advance(EStep::Throw, 0.08);
                break;

            case EStep::Throw:
                SetKey(EKeys::RightMouseButton, false);
                Advance(EStep::Breakaway, 0.15);
                break;

            case EStep::Breakaway:
                Test.TestTrue(TEXT("Tangential plate recoil breaks contact even while E is held"),
                    Pawn->GetPlateCount() == PlateCount - 1 && Mover->IsFlying() && Pawn->GetVelocity().X < -5.);
                SetKey(EKeys::E, false);
                Advance(EStep::Reengage, 0.06);
                break;

            case EStep::Reengage:
                Test.TestTrue(TEXT("Floating near the deck without E held keeps the pawn in flight"), Mover->IsFlying());
                SetKey(EKeys::E, true);
                Advance(EStep::Recapture, 0.15);
                break;

            case EStep::Recapture:
                Test.TestTrue(TEXT("A fresh E hold can reattach to the same deck after recoil"), Mover->IsMagneticallyAttached());
                SetKey(EKeys::E, false);
                return true;
            }
            return false;
        }

    private:
        enum class EStep : uint8 { Start, Attach, Walk, ChargeThrow, Throw, Breakaway, Reengage, Recapture };

        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        EStep Step = EStep::Start;
        double CheckTime = 0.3;
        double StartX = 0.;
        int32 PlateCount = 0;
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