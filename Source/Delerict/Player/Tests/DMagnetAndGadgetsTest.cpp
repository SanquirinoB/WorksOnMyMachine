#if WITH_DEV_AUTOMATION_TESTS

#include "../DDeployableGadgets.h"
#include "../DMagneticPlatform.h"
#include "../DMoverComponent.h"
#include "../DPawn.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    enum class EDTraversalStep : uint8 { Start, Attach, Move, ChargeJump, Jump, SelectHook, ChargeHook, LaunchHook, RecallHook, Deploy, Attraction, Rupture };

    class FDCheckMagnetAndGadgets final : public IAutomationLatentCommand
    {
    public:
        explicit FDCheckMagnetAndGadgets(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            auto* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            auto* Mover = Pawn ? Pawn->FindComponentByClass<UDMoverComponent>() : nullptr;
            if (FPlatformTime::Seconds() - Started > 20.)
            {
                Test.AddError(FString::Printf(TEXT("Magnet and gadget smoke stalled at step %d"), static_cast<int32>(Step)));
                return true;
            }
            if (!Mover || World->GetTimeSeconds() < Earliest)
            {
                return false;
            }

            const auto Key = [Player](const FKey& Input, EInputEvent Event, float Amount = 1.f)
            {
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(Input, Event, Event == IE_Released ? 0.f : Amount));
            };
            const auto Advance = [this, World](EDTraversalStep Next, double Delay = 0.)
            {
                Step = Next;
                Earliest = World->GetTimeSeconds() + Delay;
            };
            const FVector Location = Pawn->GetActorLocation();

            switch (Step)
            {
            case EDTraversalStep::Start:
                for (TActorIterator<ADMagneticPlatform> It(World); It; ++It)
                {
                    if (It->GetActorLocation().Z < 0.)
                    {
                        Floor = *It;
                        break;
                    }
                }
                for (TActorIterator<ADGrapplingHook> It(World); It; ++It) { Hook = *It; break; }
                if (!Floor || !Hook || !Mover->IsFlying()) { break; }
#if DELERICT_WITH_GAMEPLAY_DEBUG
                Player->ConsoleCommand(TEXT("Delerict.Debug On Magnet Jump Throws"), true);
#endif
                Key(EKeys::E, IE_Pressed);
                Advance(EDTraversalStep::Attach);
                break;

            case EDTraversalStep::Attach:
                if (!Mover->IsMagneticallyAttached()) { break; }
                BeforeMove = Location;
                Floor->AddActorWorldOffset(FVector(40., 0., 0.));
                Advance(EDTraversalStep::Move, .2);
                break;

            case EDTraversalStep::Move:
                Test.TestTrue(TEXT("A translating steel face carries the attached capsule"),
                    Mover->IsMagneticallyAttached() && FMath::IsNearlyEqual(Location.X - BeforeMove.X, 40., 1.));
                BeforeMove = Location;
                Key(EKeys::A, IE_Pressed);
                Advance(EDTraversalStep::ChargeJump, .45);
                break;

            case EDTraversalStep::ChargeJump:
                Test.TestTrue(TEXT("A walks left along the steel face"), Location.X < BeforeMove.X - 35.
                    && Mover->IsMagneticallyAttached() && FMath::IsNearlyEqual(Location.Y, BeforeMove.Y, .1));
                Key(EKeys::A, IE_Released);
                Key(EKeys::Gamepad_RightY, IE_Axis);
                Key(EKeys::SpaceBar, IE_Pressed);
                Advance(EDTraversalStep::Jump, .32);
                break;

            case EDTraversalStep::Jump:
                Key(EKeys::SpaceBar, IE_Released);
                Advance(EDTraversalStep::SelectHook, .2);
                break;

            case EDTraversalStep::SelectHook:
                Test.TestTrue(TEXT("Releasing a charged jump leaves contact with outward velocity"),
                    Mover->IsFlying() && Pawn->GetVelocity().Z > 50.);
                Key(EKeys::E, IE_Released);
                Test.TestTrue(TEXT("The grappling hook enters the throwable slot"), Pawn->TryGrabThrowable(*Hook));
                Key(EKeys::Gamepad_RightY, IE_Axis, 0.f);
                Key(EKeys::Gamepad_RightX, IE_Axis);
                Key(EKeys::Tab, IE_Pressed);
                Advance(EDTraversalStep::ChargeHook, .05);
                break;

            case EDTraversalStep::ChargeHook:
                Test.TestEqual(TEXT("Tab selects the grappling hook"), Pawn->GetSelectedThrowableType(), Hook->GetThrowableType());
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(EDTraversalStep::LaunchHook, .16);
                break;

            case EDTraversalStep::LaunchHook:
                Key(EKeys::Tab, IE_Released);
                Key(EKeys::RightMouseButton, IE_Released);
                Advance(EDTraversalStep::RecallHook, .08);
                break;

            case EDTraversalStep::RecallHook:
                Test.TestTrue(TEXT("The hook leaves the slot after a throw"), Pawn->GetGrabbedThrowable() == nullptr);
                Key(EKeys::Q, IE_Pressed);
                Advance(EDTraversalStep::Deploy, .15);
                break;

            case EDTraversalStep::Deploy:
            {
                Test.TestTrue(TEXT("Q recalls the free hook to the slot"), Pawn->GetGrabbedThrowable() == Hook);
                Key(EKeys::Q, IE_Released);
                const FVector Center = Location + FVector(195., 0., 30.);
                auto* Grenade = World->SpawnActor<ADRemoteGrenade>();
                Grenade->Launch(Center, FVector::ZeroVector, FVector::RightVector, Pawn);
                Test.TestTrue(TEXT("The remote grenade activates"), Grenade->TryActivate(*Pawn));
                Test.TestFalse(TEXT("The remote grenade consumes itself"), IsValid(Grenade));

                Device = World->SpawnActor<ADPortableBlackHoleDevice>();
                Device->Launch(Center + FVector(-35., 0., 0.), FVector::ZeroVector, FVector::RightVector, Pawn);
                Target = World->SpawnActor<ADThrowable>();
                Target->Launch(Center + FVector(60., 0., 0.), FVector::ZeroVector, FVector::RightVector, nullptr);
                Test.TestTrue(TEXT("The black hole can be activated by its deployer"), Device->TryActivate(*Pawn));
                Advance(EDTraversalStep::Attraction, .14);
                break;
            }
            case EDTraversalStep::Attraction:
                if (!Test.TestTrue(TEXT("The active field pulls a nearby plate inward"),
                    IsValid(Target) && IsValid(Device) && Target->GetVelocity().X < -30.))
                {
                    return true;
                }
                Test.TestTrue(TEXT("The black hole can be toggled off"), Device->TryActivate(*Pawn));
                Target->Launch(Device->GetActorLocation() + FVector(26., 0., 0.), FVector::ZeroVector, FVector::RightVector, nullptr);
                Test.TestTrue(TEXT("The black hole can be toggled on again"), Device->TryActivate(*Pawn));
                Advance(EDTraversalStep::Rupture, .32);
                break;

            case EDTraversalStep::Rupture:
                Test.TestTrue(TEXT("Reaching the core consumes the plate and ruptures the device"), !IsValid(Target) && !IsValid(Device));
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
        EDTraversalStep Step = EDTraversalStep::Start;
        double Earliest = 0.;
        FVector BeforeMove = FVector::ZeroVector;
        ADMagneticPlatform* Floor = nullptr;
        ADGrapplingHook* Hook = nullptr;
        ADPortableBlackHoleDevice* Device = nullptr;
        ADThrowable* Target = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDMagnetAndGadgetsTest, "Delerict.Player.MagnetAndGadgets",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDMagnetAndGadgetsTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DTestLane"), true))
    {
        return false;
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDCheckMagnetAndGadgets(*this));
    return true;
}

#endif
