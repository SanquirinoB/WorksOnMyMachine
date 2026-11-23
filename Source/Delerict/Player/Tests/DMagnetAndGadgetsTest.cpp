#if WITH_DEV_AUTOMATION_TESTS

#include "../DCompletoThrowable.h"
#include "../DDeployableGadgets.h"
#include "../DMagneticPlatform.h"
#include "../DMoverComponent.h"
#include "../DPawn.h"
#include "../DThrowSwitch.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    enum class EDTraversalStep : uint8
    {
        Start, Attach, Move, ChargeJump, Jump, SelectHook, ChargeHook, LaunchHook,
        RecallHook, ReelHook, SetAnchor, ChargeAnchor, LaunchAnchor, PullAnchor, CheckPull,
        Deploy, Attraction, Rupture, Squash
    };

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
                Key(EKeys::E, IE_Released);
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
                Test.TestTrue(TEXT("The hook leaves the player's hands after a throw"), Pawn->GetGrabbedThrowable() == nullptr);
                RecallDistance = FVector::Dist(Location, Hook->GetActorLocation());
                Key(EKeys::Q, IE_Pressed);
                Advance(EDTraversalStep::ReelHook, .045);
                break;

            case EDTraversalStep::ReelHook:
                Test.TestTrue(TEXT("The tether reels the free hook back through the world"),
                    Pawn->GetGrabbedThrowable() == nullptr && FVector::Dist(Location, Hook->GetActorLocation()) < RecallDistance);
                Key(EKeys::Q, IE_Released);
                Advance(EDTraversalStep::SetAnchor);
                break;

            case EDTraversalStep::SetAnchor:
                if (Pawn->GetGrabbedThrowable() != Hook) { break; }
                Test.TestEqual(TEXT("Recalling the hook restores its 1 kg"), Pawn->GetEffectiveMass(), 106.f);
                Pawn->QueueImpulse(-Pawn->GetVelocity() * Pawn->GetEffectiveMass());
                Anchor = World->SpawnActor<ADThrowSwitch>(Location + FVector(230., 0., 0.), FRotator::ZeroRotator);
                Anchor->Tags.Add(TEXT("Delerict.GrappleAnchor"));
                CastChecked<UPrimitiveComponent>(Anchor->GetRootComponent())->SetMobility(EComponentMobility::Movable);
                Advance(EDTraversalStep::ChargeAnchor, .06);
                break;

            case EDTraversalStep::ChargeAnchor:
                Key(EKeys::RightMouseButton, IE_Pressed);
                Advance(EDTraversalStep::LaunchAnchor, .4);
                break;

            case EDTraversalStep::LaunchAnchor:
                Key(EKeys::RightMouseButton, IE_Released);
                Advance(EDTraversalStep::PullAnchor);
                break;

            case EDTraversalStep::PullAnchor:
                if (Hook->GetAttachParentActor() != Anchor) { break; }
                Key(EKeys::Q, IE_Pressed);
                Advance(EDTraversalStep::CheckPull, .1);
                break;

            case EDTraversalStep::CheckPull:
                Test.TestTrue(TEXT("Q pulls the floating pawn toward the tagged movable anchor"), Pawn->GetVelocity().X > 100.);
                Key(EKeys::Q, IE_Released);
                Hook->Destroy();
                Anchor->Destroy();
                Pawn->QueueImpulse(-Pawn->GetVelocity() * Pawn->GetEffectiveMass());
                Advance(EDTraversalStep::Deploy, .05);
                break;

            case EDTraversalStep::Deploy:
            {
                const FVector Center = Location + FVector(195., 0., 220.);
                auto* Grenade = World->SpawnActor<ADRemoteGrenade>();
                Grenade->Launch(Center, FVector::ZeroVector, FVector::RightVector, Pawn);
                Test.TestTrue(TEXT("The remote grenade activates"), Grenade->TryActivate(*Pawn));
                Test.TestFalse(TEXT("The remote grenade consumes itself"), IsValid(Grenade));

                Device = World->SpawnActor<ADPortableBlackHoleDevice>();
                Device->Launch(Center + FVector(-35., 0., 0.), FVector::ZeroVector, FVector::RightVector, Pawn);
                Target = World->SpawnActor<ADThrowable>();
                Target->Launch(Center + FVector(60., 0., 0.), FVector::ZeroVector, FVector::RightVector, nullptr);
                LooseProp = World->SpawnActor<ADThrowSwitch>(Center + FVector(145., 0., 65.), FRotator::ZeroRotator);
                LooseProp->Tags.Add(TEXT("Delerict.GadgetTarget"));
                auto* LooseCollision = CastChecked<UPrimitiveComponent>(LooseProp->GetRootComponent());
                LooseCollision->SetMobility(EComponentMobility::Movable);
                LooseCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                BeforeAttraction = LooseProp->GetActorLocation().X;
                Test.TestTrue(TEXT("The black hole can be activated by its deployer"), Device->TryActivate(*Pawn));
                Advance(EDTraversalStep::Attraction, .14);
                break;
            }
            case EDTraversalStep::Attraction:
                if (!Test.TestTrue(FString::Printf(TEXT("The active field pulls a nearby plate inward (device valid %d, plate valid %d, plate X velocity %.1f)"), IsValid(Device), IsValid(Target), IsValid(Target) ? Target->GetVelocity().X : 0.),
                    IsValid(Target) && IsValid(Device) && Target->GetVelocity().X < -30.))
                {
                    return true;
                }
                Test.TestTrue(TEXT("The active field pulls the player and a tagged loose prop"),
                    Pawn->GetVelocity().X > 10. && LooseProp->GetActorLocation().X < BeforeAttraction - 1.);
                Test.TestTrue(TEXT("The black hole can be toggled off"), Device->TryActivate(*Pawn));
                Target->Launch(Device->GetActorLocation() + FVector(26., 0., 0.), FVector::ZeroVector, FVector::RightVector, nullptr);
                Test.TestTrue(TEXT("The black hole can be toggled on again"), Device->TryActivate(*Pawn));
                Advance(EDTraversalStep::Rupture, .32);
                break;

            case EDTraversalStep::Rupture:
            {
                Test.TestTrue(TEXT("Reaching the core consumes the plate and ruptures the device"), !IsValid(Target) && !IsValid(Device));
                auto* Food = World->SpawnActor<ADCompletoThrowable>();
                const float MassBeforePickup = Pawn->GetEffectiveMass();
                Test.TestTrue(TEXT("Completo is a grabbable 0.1 kg throwable"),
                    Pawn->TryGrabThrowable(*Food) && FMath::IsNearlyEqual(Pawn->GetEffectiveMass() - MassBeforePickup, .1f, .001f));
                FoodSwitch = World->SpawnActor<ADThrowSwitch>(FVector(370., 0., 245.), FRotator::ZeroRotator);
                FoodProjectile = World->SpawnActor<ADCompletoThrowable>();
                FoodProjectile->Launch(FVector(210., 0., 245.), FVector(900., 0., 0.), FVector::RightVector, Pawn);
                Advance(EDTraversalStep::Squash, .25);
                break;
            }

            case EDTraversalStep::Squash:
                Test.TestTrue(TEXT("A thrown Completo activates the switch and squashes on impact"),
                    FoodSwitch->IsActivated() && !FoodProjectile->CanGrab() && !FoodProjectile->GetActorEnableCollision());
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
        double RecallDistance = 0.;
        ADThrowSwitch* Anchor = nullptr;
        ADThrowSwitch* LooseProp = nullptr;
        double BeforeAttraction = 0.;
        ADThrowSwitch* FoodSwitch = nullptr;
        ADCompletoThrowable* FoodProjectile = nullptr;
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
