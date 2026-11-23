#if WITH_DEV_AUTOMATION_TESTS

#include "../DRoomProgress.h"
#include "../../Player/DMoverComponent.h"
#include "../../Player/DPawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "UnrealClient.h"

namespace
{
    enum class EDRouteStep : uint8 { Start, Rise, Checkpoint, Descend, Hazard, Retry, Exit };

    class FDPlayLabRoute final : public IAutomationLatentCommand
    {
    public:
        explicit FDPlayLabRoute(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            auto* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            if (FPlatformTime::Seconds() - Started > 25.)
            {
                Test.AddError(FString::Printf(TEXT("Lab route stalled at step %d with pawn position %s"),
                    static_cast<int32>(Step), Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("missing")));
                return true;
            }

            const auto* Mover = Pawn ? Pawn->FindComponentByClass<UDMoverComponent>() : nullptr;
            if (!Mover || !Mover->IsFlying())
            {
                return false;
            }

            const FVector Position = Pawn->GetActorLocation();
            const auto SetDrift = [Pawn](const FVector& Velocity)
            {
                Pawn->QueueImpulse((Velocity - Pawn->GetVelocity()) * Pawn->GetEffectiveMass());
            };
            switch (Step)
            {
            case EDRouteStep::Start:
                Test.TestTrue(TEXT("Lab uses its start prefab"), FMath::Abs(Position.X) < 30.);
                OriginalPawn = Pawn;
                SetDrift(FVector(440., 0., 110.));
                Step = EDRouteStep::Rise;
                break;
            case EDRouteStep::Rise:
                if (Position.Z >= 190.)
                {
                    SetDrift(FVector(500., 0., 0.));
                    Step = EDRouteStep::Checkpoint;
                }
                break;
            case EDRouteStep::Checkpoint:
                if (Position.X >= 1560.)
                {
                    Test.TestTrue(TEXT("The pawn crossed the gap above the landing platform"), Position.Z > 175.);
                    if (FParse::Param(FCommandLine::Get(), TEXT("DelerictCaptureLab")))
                    {
                        FScreenshotRequest::RequestScreenshot(
                            FPaths::ProjectSavedDir() / TEXT("Validation/LevelDesign/LabCheckpoint.png"), false, false);
                    }
                    SetDrift(FVector(0., 0., -180.));
                    Step = EDRouteStep::Descend;
                }
                break;
            case EDRouteStep::Descend:
                if (Position.Z <= 105.)
                {
                    SetDrift(FVector(320., 0., 0.));
                    Step = EDRouteStep::Hazard;
                }
                break;
            case EDRouteStep::Hazard:
                if (Pawn != OriginalPawn.Get())
                {
                    Test.TestTrue(TEXT("Hazard respawns the player at the activated checkpoint"),
                        FMath::Abs(Position.X - 1450.) < 20. && FMath::Abs(Position.Y) < 1.);
                    SetDrift(FVector(0., 0., 160.));
                    Step = EDRouteStep::Retry;
                }
                break;
            case EDRouteStep::Retry:
                if (Position.Z >= 155.)
                {
                    SetDrift(FVector(440., 0., 0.));
                    Step = EDRouteStep::Exit;
                }
                break;
            case EDRouteStep::Exit:
                if (World->GetSubsystem<UDRoomProgress>()->HasCompletedRoom(*Player))
                {
                    Test.TestTrue(TEXT("Player crossed the second hazard and reached the exit"),
                        Position.X > 2150. && FMath::Abs(Position.Y) < 1.);
                    return true;
                }
                break;
            }
            return false;
        }

    private:
        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        EDRouteStep Step = EDRouteStep::Start;
        TWeakObjectPtr<ADPawn> OriginalPawn;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLabRouteTest, "Delerict.LevelDesign.PlayableRoute",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDLabRouteTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DLevelDesignLab")))
    {
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDPlayLabRoute(*this));
    return true;
}

#endif
