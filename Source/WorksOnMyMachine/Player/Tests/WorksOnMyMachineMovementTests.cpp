#if WITH_DEV_AUTOMATION_TESTS

#include "../WorksOnMyMachinePawn.h"
#include "Camera/PlayerCameraManager.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    enum class EMovementPhase { Spawn, Move, Brake, Jump, Wall, Reverse };

    class FCheckLaneMovement final : public IAutomationLatentCommand
    {
    public:
        explicit FCheckLaneMovement(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            if (FPlatformTime::Seconds() - StartTime > 20.)
            {
                Test.AddError(TEXT("The player did not complete the movement check."));
                return true;
            }

            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? GEngine->GetFirstLocalPlayerController(World) : nullptr;
            AWorksOnMyMachinePawn* Pawn = Player ? Cast<AWorksOnMyMachinePawn>(Player->GetPawn()) : nullptr;
            UCharacterMoverComponent* Mover = Pawn ? Pawn->FindComponentByClass<UCharacterMoverComponent>() : nullptr;
            if (!Mover || !Player->PlayerCameraManager)
            {
                return false;
            }

            const FVector Position = Pawn->GetActorLocation();
            const FMinimalViewInfo& View = Player->PlayerCameraManager->GetCameraCacheView();
            const double Now = World->GetTimeSeconds();
            const double Elapsed = Now - PhaseStarted;
            const auto Advance = [this, Now](EMovementPhase Next)
            {
                Phase = Next;
                PhaseStarted = Now;
            };
            const auto SendKey = [Player](FKey Key, EInputEvent Event)
            {
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Event == IE_Pressed ? 1.f : 0.f));
            };

            switch (Phase)
            {
            case EMovementPhase::Spawn:
                if (!Mover->IsOnGround()) { return false; }
                StartPosition = Position;
                StartView = View;
                {
                    int32 Width = 0;
                    int32 Height = 0;
                    Player->GetViewportSize(Width, Height);
                    FVector2D Screen;
                    const FVector Target = View.Location + View.Rotation.Vector() * 1500.;
                    Test.TestTrue(TEXT("The camera view has a centered projection"),
                        Width > 0 && Height > 0 && Player->ProjectWorldLocationToScreen(Target, Screen)
                        && Screen.Equals(FVector2D(Width * .5, Height * .5), 3.));
                }
                Player->SetControlRotation(FRotator(-12., 153., 0.));
                Test.TestTrue(TEXT("Mover uses 80 percent of world gravity"),
                    FMath::IsNearlyEqual(Mover->GetGravityAcceleration().Z, World->GetGravityZ() * .8, 1.));
                SendKey(EKeys::D, IE_Pressed);
                Advance(EMovementPhase::Move);
                break;

            case EMovementPhase::Move:
                if (Elapsed < .45) { return false; }
                Test.TestTrue(TEXT("D moves right along the lane"), Position.X > StartPosition.X + 60.
                    && FMath::IsNearlyEqual(Position.Y, StartPosition.Y, .5) && Mover->IsOnGround());
                Test.TestTrue(TEXT("The camera follows X and preserves its lateral framing"),
                    View.Location.X > StartView.Location.X + 30. && View.Location.Y > Position.Y
                    && FMath::IsNearlyEqual(View.Rotation.Yaw, -90., .1)
                    && View.Rotation.Equals(StartView.Rotation, .1)
                    && View.ProjectionMode == StartView.ProjectionMode);
                SendKey(EKeys::D, IE_Released);
                Advance(EMovementPhase::Brake);
                break;

            case EMovementPhase::Brake:
                if (Elapsed < .35) { return false; }
                Test.TestTrue(TEXT("Releasing D stops the pawn"), Pawn->GetVelocity().Size2D() < 1.);
                TakeoffHeight = Position.Z;
                SendKey(EKeys::SpaceBar, IE_Pressed);
                Advance(EMovementPhase::Jump);
                break;

            case EMovementPhase::Jump:
            {
                JumpHeight = FMath::Max(JumpHeight, Position.Z - TakeoffHeight);
                if (Mover->IsAirborne() && !bBecameAirborne)
                {
                    bBecameAirborne = true;
                    SendKey(EKeys::SpaceBar, IE_Released);
                }
                const bool bLanded = bBecameAirborne && Mover->IsOnGround()
                    && FMath::IsNearlyEqual(Position.Z, TakeoffHeight, 3.);
                if (!bLanded && Elapsed < 2.5) { return false; }
                Test.TestTrue(FString::Printf(TEXT("Space jumps and returns to the floor (apex %.1f cm, floor %.1f to %.1f cm)"),
                    JumpHeight, TakeoffHeight, Position.Z), bLanded && JumpHeight > 150.);
                SendKey(EKeys::D, IE_Pressed);
                Advance(EMovementPhase::Wall);
                break;
            }

            case EMovementPhase::Wall:
                if (Elapsed < 1.25) { return false; }
                Test.TestTrue(FString::Printf(TEXT("Capsule stops at the wall (X %.1f cm)"), Position.X),
                    Position.X > 425. && Position.X < 460.);
                WallX = Position.X;
                SendKey(EKeys::D, IE_Released);
                SendKey(EKeys::A, IE_Pressed);
                Advance(EMovementPhase::Reverse);
                break;

            case EMovementPhase::Reverse:
                if (Elapsed < .5) { return false; }
                Test.TestTrue(TEXT("A moves left along the lane"), Position.X < WallX - 60.
                    && FMath::IsNearlyEqual(Position.Y, StartPosition.Y, .5));
                Test.TestTrue(TEXT("Changing direction preserves the camera angle"),
                    View.Rotation.Equals(StartView.Rotation, .1) && View.ProjectionMode == StartView.ProjectionMode);
                SendKey(EKeys::A, IE_Released);
                return true;
            }
            return false;
        }

    private:
        FAutomationTestBase& Test;
        const double StartTime = FPlatformTime::Seconds();
        EMovementPhase Phase = EMovementPhase::Spawn;
        double PhaseStarted = 0.;
        FVector StartPosition = FVector::ZeroVector;
        FMinimalViewInfo StartView;
        double TakeoffHeight = 0.;
        double JumpHeight = 0.;
        double WallX = 0.;
        bool bBecameAirborne = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorksOnMyMachineGroundMovementTest, "WorksOnMyMachine.Player.GroundMovement",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FWorksOnMyMachineGroundMovementTest::RunTest(const FString& Parameters)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_WorksOnMyMachine")))
    {
        AddError(TEXT("Could not open the movement test lane."));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FCheckLaneMovement(*this));
    return true;
}

#endif
