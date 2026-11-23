#if WITH_DEV_AUTOMATION_TESTS

#include "../DCrosshairHUD.h"
#include "../DMoverComponent.h"
#include "../DPawn.h"
#include "../DPlayerTuning.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Templates/UnrealTemplate.h"
#include "Tests/AutomationCommon.h"

namespace
{
    void SendKey(APlayerController& Player, const FKey& Key, EInputEvent Event, float Value = 1.f)
    {
        Player.InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Event == IE_Released ? 0.f : Value));
    }

    enum class EDFlightTestPhase : uint8 { Start, Rest, Controls, Drift, Wall, Cancel };
    struct FDFlightInputEvents
    {
        TArray<EDChargePhase> Jump;
        TArray<EDChargePhase> Throw;
        FVector JumpReleaseAim = FVector::ZeroVector;
        int32 Grabs = 0;
        int32 Selections = 0;
    };

    class FDCheckFlight final : public IAutomationLatentCommand
    {
    public:
        explicit FDCheckFlight(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            if (FPlatformTime::Seconds() - Started > 20.)
            {
                Test.AddError(FString::Printf(TEXT("Flight check stalled in phase %d"), static_cast<int32>(Phase)));
                return true;
            }

            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            auto* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            auto* Mover = Pawn ? Pawn->FindComponentByClass<UDMoverComponent>() : nullptr;
            if (!Mover || !Player->PlayerCameraManager || !Mover->IsFlying())
            {
                return false;
            }

            const double Now = World->GetTimeSeconds();
            const double Elapsed = Now - PhaseStarted;
            const FVector Location = Pawn->GetActorLocation();
            const FVector Velocity = Pawn->GetVelocity();
            const auto Advance = [this, Now](EDFlightTestPhase Next)
            {
                Phase = Next;
                PhaseStarted = Now;
            };

            switch (Phase)
            {
            case EDFlightTestPhase::Start:
            {
                Start = Location;
                const auto* Capsule = Cast<UCapsuleComponent>(Pawn->GetRootComponent());
                const auto* Mesh = Pawn->FindComponentByClass<USkeletalMeshComponent>();
                Test.TestTrue(TEXT("The tuned capsule is the swept root"), Capsule && Mover->GetUpdatedComponent() == Capsule
                    && Capsule->GetUnscaledCapsuleRadius() == Pawn->GetTuning().CapsuleRadius
                    && Capsule->GetUnscaledCapsuleHalfHeight() == Pawn->GetTuning().CapsuleHalfHeight);
                Test.TestTrue(TEXT("The player mesh has collision disabled"),
                    Mesh && Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
                Test.TestTrue(TEXT("The player starts with zero gravity"), Mover->GetGravityAcceleration().IsNearlyZero());
                Test.TestTrue(TEXT("The game mode spawns the crosshair HUD"), Player->GetHUD<ADCrosshairHUD>() != nullptr);
                Pawn->OnJumpCharge.AddLambda([Events = Events](EDChargePhase Charge, const FVector& Aim)
                {
                    Events->Jump.Add(Charge);
                    if (Charge == EDChargePhase::Released)
                    {
                        Events->JumpReleaseAim = Aim;
                    }
                });
                Pawn->OnThrowCharge.AddLambda([Events = Events](EDChargePhase Charge, const FVector&) { Events->Throw.Add(Charge); });
                Pawn->OnGrabRequested.AddLambda([Events = Events] { ++Events->Grabs; });
                Pawn->OnSelectThrowableRequested.AddLambda([Events = Events] { ++Events->Selections; });
                AActor* Wall = World->SpawnActor<AActor>();
                auto* Collision = NewObject<UBoxComponent>(Wall);
                Wall->SetRootComponent(Collision);
                Collision->SetBoxExtent(FVector(6., 180., 300.));
                Collision->SetCollisionProfileName(TEXT("BlockAll"));
                Collision->SetWorldLocation(Start + FVector(320., 0., 160.));
                Collision->RegisterComponent();
#if DELERICT_WITH_GAMEPLAY_DEBUG
                Player->ConsoleCommand(TEXT("Delerict.Debug On Flight Aim Collision"), true);
#endif
                Advance(EDFlightTestPhase::Rest);
                break;
            }
            case EDFlightTestPhase::Rest:
                if (Elapsed < .2) { break; }
                Test.TestTrue(TEXT("The pawn remains at rest without impulses"), Location.Equals(Start, .1) && Velocity.IsNearlyZero());
                Player->SetControlRotation(FRotator(25., 135., 15.));
                SendKey(*Player, EKeys::D, IE_Pressed);
                SendKey(*Player, EKeys::W, IE_Pressed);
                SendKey(*Player, EKeys::LeftShift, IE_Pressed);
                SendKey(*Player, EKeys::SpaceBar, IE_Pressed);
                SendKey(*Player, EKeys::RightMouseButton, IE_Pressed);
                SendKey(*Player, EKeys::E, IE_Pressed);
                SendKey(*Player, EKeys::Q, IE_Pressed);
                Advance(EDFlightTestPhase::Controls);
                break;

            case EDFlightTestPhase::Controls:
            {
                if (Elapsed < .12) { break; }
                Test.TestTrue(TEXT("Movement and Run are exposed without steering free flight"),
                    Pawn->GetMoveIntent().X > .65 && Pawn->GetMoveIntent().Y > .65 && Pawn->IsRunHeld()
                    && Location.Equals(Start, .1) && Velocity.IsNearlyZero());
                Test.TestTrue(TEXT("Grab and selection each fire once"), Events->Grabs == 1 && Events->Selections == 1);
                SendKey(*Player, EKeys::Gamepad_RightY, IE_Axis, 1.f);
                SendKey(*Player, EKeys::SpaceBar, IE_Released);
                SendKey(*Player, EKeys::RightMouseButton, IE_Released);
                SendKey(*Player, EKeys::D, IE_Released);
                SendKey(*Player, EKeys::W, IE_Released);
                SendKey(*Player, EKeys::LeftShift, IE_Released);
                SendKey(*Player, EKeys::E, IE_Released);
                SendKey(*Player, EKeys::Q, IE_Released);

                const float Mass = Pawn->GetEffectiveMass();
                Pawn->QueueImpulse(FVector(150., 500., 30.) * Mass);
                {
                    auto& Tuning = const_cast<UDPlayerTuning&>(Pawn->GetTuning());
                    TGuardValue<float> TemporaryMass(Tuning.PlayerMass, Mass * 2.f);
                    Pawn->QueueImpulse(FVector(90., -200., 40.) * (Mass * 2.f));
                }
                Advance(EDFlightTestPhase::Drift);
                break;
            }
            case EDFlightTestPhase::Drift:
            {
                if (Elapsed < .4) { break; }
                const TArray<EDChargePhase> Released = { EDChargePhase::Pressed, EDChargePhase::Released };
                Test.TestTrue(TEXT("Jump and Throw each report a single press and release"),
                    Events->Jump == Released && Events->Throw == Released);
                Test.TestTrue(TEXT("The released jump uses the current right-stick aim"), Events->JumpReleaseAim.Z > .95);
                Test.TestTrue(TEXT("Impulses project into the lane and apply once using the mass at each event"),
                    Velocity.Equals(FVector(240., 0., 70.), 1.) && FMath::IsNearlyEqual(Location.Y, Start.Y, .1));
                Test.TestTrue(TEXT("The capsule keeps its configured rotation when aiming"),
                    Pawn->GetActorRotation().Equals(Pawn->GetTuning().LockedRotation, .1));
                const auto* Camera = Pawn->FindComponentByClass<UGameplayCameraComponent>();
                const double ExpectedLead = Pawn->GetTuning().CameraLookAheadTime * Velocity.X;
                Test.TestTrue(TEXT("The camera mount follows the tuning look-ahead"),
                    Camera && FMath::IsNearlyEqual(Camera->GetComponentLocation().X - Location.X, ExpectedLead, .5));
                Advance(EDFlightTestPhase::Wall);
                break;
            }
            case EDFlightTestPhase::Wall:
                if (Elapsed < 1.3) { break; }
                Test.TestTrue(TEXT("The swept capsule stops at the wall and keeps sliding along it"),
                    FMath::IsNearlyEqual(Location.X - Start.X, 280., 3.) && FMath::Abs(Velocity.X) < 1.
                    && Location.Z > Start.Z + 90. && Velocity.Z > 35. && FMath::IsNearlyEqual(Location.Y, Start.Y, .1));
                SendKey(*Player, EKeys::SpaceBar, IE_Pressed);
                Advance(EDFlightTestPhase::Cancel);
                break;

            case EDFlightTestPhase::Cancel:
                if (Elapsed < .08) { break; }
                Player->UnPossess();
                Test.TestTrue(TEXT("Unpossessing cancels a pending charge without committing it"),
                    Events->Jump.Num() == 4 && Events->Jump.Last() == EDChargePhase::Canceled);
                SendKey(*Player, EKeys::SpaceBar, IE_Released);
                Player->Possess(Pawn);
#if DELERICT_WITH_GAMEPLAY_DEBUG
                Player->ConsoleCommand(TEXT("Delerict.Debug Off"), true);
#endif
                return true;
            }
            return false;
        }

    private:
        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        EDFlightTestPhase Phase = EDFlightTestPhase::Start;
        double PhaseStarted = 0.;
        FVector Start = FVector::ZeroVector;
        TSharedRef<FDFlightInputEvents> Events = MakeShared<FDFlightInputEvents>();
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDInertialFlightTest, "Delerict.Player.InertialFlight",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDInertialFlightTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DTestLane")))
    {
        AddError(TEXT("Could not open the movement test lane."));
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDCheckFlight(*this));
    return true;
}

#endif
