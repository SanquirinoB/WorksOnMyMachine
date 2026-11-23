#if WITH_DEV_AUTOMATION_TESTS

#include "../DRoomProgress.h"
#include "../../Player/DPawn.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "UnrealClient.h"

namespace
{
    enum class EDRouteStep : uint8 { Start, Approach, Gap, Checkpoint, Hazard, Retry, Exit };

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
                if (Player)
                {
                    Key(*Player, EKeys::D, IE_Released);
                    Key(*Player, EKeys::SpaceBar, IE_Released);
                }
                return true;
            }
            if (!Pawn)
            {
                return false;
            }

            const auto* Mover = Pawn->FindComponentByClass<UCharacterMoverComponent>();
            const FVector Position = Pawn->GetActorLocation();
            const bool bGrounded = Mover && Mover->IsOnGround();
            const auto Jump = [&] { Key(*Player, EKeys::SpaceBar, IE_Pressed); };
            const auto ReleaseJump = [&] { Key(*Player, EKeys::SpaceBar, IE_Released); };
            switch (Step)
            {
            case EDRouteStep::Start:
                if (bGrounded)
                {
                    Test.TestTrue(TEXT("Lab uses its start prefab"), FMath::Abs(Position.X) < 30.);
                    OriginalPawn = Pawn;
                    Key(*Player, EKeys::D, IE_Pressed);
                    Step = EDRouteStep::Approach;
                }
                break;
            case EDRouteStep::Approach:
                if (Position.X >= 560.)
                {
                    TakeoffZ = Position.Z;
                    TakeoffX = Position.X;
                    Jump();
                    Step = EDRouteStep::Gap;
                }
                break;
            case EDRouteStep::Gap:
                Apex = FMath::Max(Apex, Position.Z - TakeoffZ);
                if (bGrounded && Position.X > 840.)
                {
                    ReleaseJump();
                    Test.TestTrue(TEXT("The running jump reaches the elevated platform"), Apex > 100. && Position.Z > 130.);
                    Test.AddInfo(FString::Printf(TEXT("Running jump apex: %.1f cm, takeoff to landing: %.1f cm"),
                        Apex, Position.X - TakeoffX));
                    Step = EDRouteStep::Checkpoint;
                }
                break;
            case EDRouteStep::Checkpoint:
                if (Position.X > 1530.)
                {
                    if (FParse::Param(FCommandLine::Get(), TEXT("DelerictCaptureLab")))
                    {
                        FScreenshotRequest::RequestScreenshot(
                            FPaths::ProjectSavedDir() / TEXT("Validation/LevelDesign/LabCheckpoint.png"), false, false);
                    }
                    Step = EDRouteStep::Hazard;
                }
                break;
            case EDRouteStep::Hazard:
                if (Pawn != OriginalPawn.Get())
                {
                    Key(*Player, EKeys::D, IE_Released);
                    Test.TestTrue(TEXT("Hazard respawns the player at the activated checkpoint"),
                        FMath::Abs(Position.X - 1450.) < 20. && FMath::Abs(Position.Y) < 1.);
                    Key(*Player, EKeys::D, IE_Pressed);
                    Step = EDRouteStep::Retry;
                }
                break;
            case EDRouteStep::Retry:
                if (Position.X >= 1610.)
                {
                    Jump();
                    Step = EDRouteStep::Exit;
                }
                break;
            case EDRouteStep::Exit:
                if (bGrounded && Position.X > 1880.)
                {
                    ReleaseJump();
                }
                if (World->GetSubsystem<UDRoomProgress>()->HasCompletedRoom(*Player))
                {
                    Key(*Player, EKeys::D, IE_Released);
                    Test.TestTrue(TEXT("Player crossed the second hazard and reached the exit"),
                        Position.X > 2150. && FMath::Abs(Position.Y) < 1.);
                    return true;
                }
                break;
            }
            return false;
        }

    private:
        static void Key(APlayerController& Player, FKey InKey, EInputEvent Event)
        {
            Player.InputKey(FInputKeyEventArgs::CreateSimulated(InKey, Event, Event == IE_Pressed ? 1.f : 0.f));
        }

        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        EDRouteStep Step = EDRouteStep::Start;
        TWeakObjectPtr<ADPawn> OriginalPawn;
        double TakeoffZ = 0.;
        double TakeoffX = 0.;
        double Apex = 0.;
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
