#if WITH_DEV_AUTOMATION_TESTS

#include "../DPawn.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
    class FDCheckBodoqueShortcut final : public IAutomationLatentCommand
    {
    public:
        explicit FDCheckBodoqueShortcut(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            if (FPlatformTime::Seconds() - Started > 15.)
            {
                Test.AddError(TEXT("J did not take the moving pawn to the escape-pod room entrance."));
                return true;
            }

            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            ADPawn* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            if (!Pawn || World->GetTimeSeconds() < 1.) { return false; }

            if (!OriginalPawn.IsValid())
            {
                TArray<AActor*> Arrivals;
                UGameplayStatics::GetAllActorsWithTag(World, TEXT("Delerict.BodoqueRoomStart"), Arrivals);
                if (!Test.TestEqual(TEXT("The campaign has one shortcut arrival marker"), Arrivals.Num(), 1)) { return true; }
                Target = Arrivals[0]->GetActorLocation();
                if (!Test.TestTrue(TEXT("The player starts outside the escape-pod room"),
                    FVector::Dist(Pawn->GetActorLocation(), Target) > 500.)) { return true; }
                OriginalPawn = Pawn;
                PlateCount = Pawn->GetPlateCount();
                Pawn->QueueImpulse(FVector::UpVector * (450.f * Pawn->GetEffectiveMass()));
                return false;
            }

            if (!bPressed)
            {
                if (Pawn->GetVelocity().Size() < 50.) { return false; }
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Pressed, 1.f));
                bPressed = true;
                return false;
            }

            if (!Pawn->GetActorLocation().Equals(Target, 8.) || !Pawn->GetVelocity().IsNearlyZero(1.)) { return false; }
            Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::J, IE_Released, 0.f));
            Test.TestTrue(TEXT("The teleport retains the possessed pawn"), Pawn == OriginalPawn.Get());
            Test.TestEqual(TEXT("The teleport keeps the inventory"), Pawn->GetPlateCount(), PlateCount);
            return true;
        }

    private:
        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        TWeakObjectPtr<ADPawn> OriginalPawn;
        FVector Target = FVector::ZeroVector;
        int32 PlateCount = 0;
        bool bPressed = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDBodoqueRoomShortcutTest, "Delerict.Player.BodoqueRoomShortcut",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDBodoqueRoomShortcutTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DCampaign"))) { return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FDCheckBodoqueShortcut(*this));
    return true;
}

#endif
