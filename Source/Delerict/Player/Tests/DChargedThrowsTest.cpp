#if WITH_DEV_AUTOMATION_TESTS

#include "../DPawn.h"
#include "../DPlayerTuning.h"
#include "../DThrowable.h"
#include "../DThrowSwitch.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    enum class EDThrowStep : uint8
    {
        Start, Grab, ChangeSelection, Cancel, Charge, Launch, Hit, Remote, Block,
        CheckBlocked, Tap, CheckTap, Empty, Finish
    };

    class FDCheckChargedThrows final : public IAutomationLatentCommand
    {
    public:
        explicit FDCheckChargedThrows(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            auto* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            if (FPlatformTime::Seconds() - Started > 20.)
            {
                Test.AddError(FString::Printf(TEXT("Charged throw test stalled at step %d"), static_cast<int32>(Step)));
                return true;
            }
            if (!Pawn || World->GetTimeSeconds() < Earliest)
            {
                return false;
            }

            const auto Key = [Player](const FKey& Input, EInputEvent Event)
            {
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(Input, Event, Event == IE_Released ? 0.f : 1.f));
            };
            const auto Advance = [this, World](EDThrowStep Next, double Delay = 0.)
            {
                Step = Next;
                Earliest = World->GetTimeSeconds() + Delay;
            };

            switch (Step)
            {
            case EDThrowStep::Start:
            {
                for (TActorIterator<ADThrowable> It(World); It; ++It)
                {
                    if (It->GetThrowableType() == TEXT("Heavy"))
                    {
                        Heavy = *It;
                        break;
                    }
                }
                for (TActorIterator<ADThrowSwitch> It(World); It; ++It)
                {
                    Target = *It;
                    break;
                }
                if (!Test.TestNotNull(TEXT("Saved heavy throwable fixture"), Heavy)
                    || !Test.TestNotNull(TEXT("Saved switch fixture"), Target))
                {
                    return true;
                }

                Test.TestEqual(TEXT("The player begins with three plates"), Pawn->GetPlateCount(), 3);
                Test.TestEqual(TEXT("Base mass is 90 kg and plates add 15 kg"), Pawn->GetEffectiveMass(), 105.f);
                RemoteWitness = World->SpawnActor<ADThrowSwitch>(FVector(0., 500., 200.), FRotator::ZeroRotator);
                Heavy->bRemoteActivatable = true;
                Heavy->OnRemoteActivated.AddDynamic(RemoteWitness, &ADThrowSwitch::Activate);
                Heavy->Launch(Pawn->GetActorLocation() + FVector(75., 0., 0.),
                    FVector(-72., 0., 0.), FVector::RightVector, nullptr);
                Pawn->QueueImpulse(FVector(40., 0., 0.) * Pawn->GetEffectiveMass());
#if DELERICT_WITH_GAMEPLAY_DEBUG
                Player->ConsoleCommand(TEXT("Delerict.Debug On Throws Flight"), true);
#endif
                Key(EKeys::F, IE_Pressed);
                Advance(EDThrowStep::Grab, .10);
                break;
            }
            case EDThrowStep::Grab:
                Test.TestTrue(TEXT("F grabs the moving item into the slot"),
                    Pawn->GetGrabbedThrowable() == Heavy && Heavy->IsHidden());
                Test.TestEqual(TEXT("The held item adds 8 kg"), Pawn->GetEffectiveMass(), 113.f);
                Test.TestTrue(TEXT("The grab shares momentum with the already queued player impulse"),
                    FMath::IsNearlyEqual(Pawn->GetVelocity().X, (105. * 40. - 8. * 72.) / 113., 1.));
                Key(EKeys::F, IE_Released);
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(EDThrowStep::ChangeSelection, .12);
                break;

            case EDThrowStep::ChangeSelection:
                Test.TestTrue(TEXT("Holding RMB builds charge"), Pawn->GetThrowCharge() > .05f);
                Key(EKeys::Tab, IE_Pressed);
                Advance(EDThrowStep::Cancel, .08);
                break;

            case EDThrowStep::Cancel:
                Test.TestTrue(TEXT("Tab selects the held item and cancels the active charge"),
                    Pawn->GetSelectedThrowableType() == TEXT("Heavy") && Pawn->GetThrowCharge() == 0.f);
                Key(EKeys::Tab, IE_Released);
                Key(EKeys::RightMouseButton, IE_Released);
                Advance(EDThrowStep::Charge, .08);
                break;

            case EDThrowStep::Charge:
                Test.TestTrue(TEXT("Releasing a canceled throw keeps both selections"),
                    Pawn->GetPlateCount() == 3 && Pawn->GetGrabbedThrowable() == Heavy);
                Key(EKeys::Gamepad_RightX, IE_Axis);
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(EDThrowStep::Launch, Pawn->GetTuning().ThrowChargeTime + .12);
                break;

            case EDThrowStep::Launch:
                Test.TestTrue(TEXT("Charge saturates at one"), FMath::IsNearlyEqual(Pawn->GetThrowCharge(), 1.f));
                BeforeThrow = Pawn->GetVelocity().X;
                Key(EKeys::RightMouseButton, IE_Released);
                Advance(EDThrowStep::Hit);
                break;

            case EDThrowStep::Hit:
                if (Pawn->GetGrabbedThrowable()) { break; }
                Test.TestEqual(TEXT("Launching the held item removes only its mass"), Pawn->GetEffectiveMass(), 105.f);
                Test.TestTrue(TEXT("The full throw gives the item and pawn the configured separation speed"),
                    FMath::IsNearlyEqual(Heavy->GetVelocity().X - Pawn->GetVelocity().X, 1600., 15.));
                Test.TestTrue(TEXT("The full throw conserves their shared momentum"),
                    FMath::IsNearlyEqual(105. * Pawn->GetVelocity().X + 8. * Heavy->GetVelocity().X, 113. * BeforeThrow, 55.));
                Advance(EDThrowStep::Remote);
                break;

            case EDThrowStep::Remote:
            {
                if (!Target->IsActivated()) { break; }
                Test.TestTrue(TEXT("The impact activates the switch and leaves the item recoverable"), Heavy->CanGrab());
                Key(EKeys::Q, IE_Pressed);
                Pawn->QueueImpulse(-Pawn->GetVelocity() * Pawn->GetEffectiveMass());
                Blocker = World->SpawnActor<AActor>();
                auto* Collision = NewObject<UBoxComponent>(Blocker);
                Blocker->SetRootComponent(Collision);
                Collision->SetBoxExtent(FVector(2., 80., 80.));
                Collision->SetCollisionProfileName(TEXT("BlockAll"));
                Collision->SetWorldLocation(Pawn->GetActorLocation() + FVector(50., 0., 0.));
                Collision->RegisterComponent();
                Advance(EDThrowStep::Block, .10);
                break;
            }
            case EDThrowStep::Block:
                Test.TestTrue(TEXT("Q reaches the deployed item"), RemoteWitness->IsActivated());
                // Move the rebounding fixture out of the firing lane before testing isolated plate throws.
                Heavy->Launch(Pawn->GetActorLocation() + FVector(0., 0., 250.),
                    FVector::ZeroVector, FVector::RightVector, nullptr);
                Key(EKeys::Q, IE_Released);
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(EDThrowStep::CheckBlocked, .06);
                break;

            case EDThrowStep::CheckBlocked:
                Key(EKeys::RightMouseButton, IE_Released);
                Advance(EDThrowStep::Tap, .10);
                break;

            case EDThrowStep::Tap:
                Test.TestTrue(TEXT("A blocked launch spends no plate and applies no recoil"),
                    Pawn->GetPlateCount() == 3 && Pawn->GetVelocity().IsNearlyZero(1.));
                Blocker->Destroy();
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(EDThrowStep::CheckTap, .04);
                break;

            case EDThrowStep::CheckTap:
                Key(EKeys::RightMouseButton, IE_Released);
                Advance(EDThrowStep::Empty, .16);
                break;

            case EDThrowStep::Empty:
                --ExpectedPlates;
                Test.TestEqual(TEXT("Each successful tap throw spends one plate"), Pawn->GetPlateCount(), ExpectedPlates);
                Test.TestEqual(TEXT("Each plate release removes 5 kg"), Pawn->GetEffectiveMass(), 90.f + 5.f * ExpectedPlates);
                if (ExpectedPlates == 2)
                {
                    Test.TestTrue(TEXT("A tap gives the player less recoil than the full heavy throw"),
                        Pawn->GetVelocity().X < -10. && Pawn->GetVelocity().X > -45.);
                }
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(ExpectedPlates > 0 ? EDThrowStep::CheckTap : EDThrowStep::Finish, .08);
                break;

            case EDThrowStep::Finish:
                Test.TestEqual(TEXT("An empty selection cannot build charge"), Pawn->GetThrowCharge(), 0.f);
                Test.TestTrue(TEXT("The depleted selection is empty"), Pawn->GetSelectedThrowableType().IsNone());
                Key(EKeys::RightMouseButton, IE_Released);
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
        EDThrowStep Step = EDThrowStep::Start;
        double Earliest = 0.;
        double BeforeThrow = 0.;
        int32 ExpectedPlates = 3;
        ADThrowable* Heavy = nullptr;
        ADThrowSwitch* Target = nullptr;
        ADThrowSwitch* RemoteWitness = nullptr;
        AActor* Blocker = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDChargedThrowsTest, "Delerict.Player.ChargedThrows",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDChargedThrowsTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DTestLane")))
    {
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDCheckChargedThrows(*this));
    return true;
}

#endif
