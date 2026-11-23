#if WITH_DEV_AUTOMATION_TESTS

#include "../DPawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "UnrealClient.h"

namespace
{
    class FDFlyHangar final : public IAutomationLatentCommand
    {
    public:
        explicit FDFlyHangar(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            if (FPlatformTime::Seconds() - Started > 45.)
            {
                Test.AddError(TEXT("The pawn did not traverse the hangar from the start room to the end wall."));
                return true;
            }
            UWorld* World = AutomationCommon::GetAnyGameWorld();
            APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
            ADPawn* Pawn = Player ? Cast<ADPawn>(Player->GetPawn()) : nullptr;
            const auto* Mover = Pawn ? Pawn->FindComponentByClass<UCharacterMoverComponent>() : nullptr;
            if (!Mover || !Player->PlayerCameraManager) { return false; }
            const FVector Position = Pawn->GetActorLocation();
            if (!bMoving)
            {
                if (!Mover->IsFlying() || World->GetTimeSeconds() < 1.5) { return false; }
                const auto* Capsule = Pawn->FindComponentByClass<UCapsuleComponent>();
                if (!Test.TestNotNull(TEXT("Player capsule"), Capsule)) { return true; }
                int32 Bodoques = 0;
                int32 Completos = 0;
                for (TActorIterator<AActor> It(World); It; ++It)
                {
                    if (It->GetClass()->GetFName() == TEXT("BP_DBodoque_C"))
                    {
                        const auto* Body = It->FindComponentByClass<USkeletalMeshComponent>();
                        Test.TestTrue(TEXT("Bodoque has his animated mesh in the start room"),
                            Body && Body->GetSkeletalMeshAsset() && Body->IsPlaying() && !It->IsHidden());
                        ++Bodoques;
                    }
                    if (It->ActorHasTag(TEXT("Delerict.CompletoTween")))
                    {
                        TArray<AActor*> AttachedProps;
                        It->GetAttachedActors(AttachedProps);
                        const AActor* Prop = AttachedProps.Num() == 1 ? AttachedProps[0] : nullptr;
                        const auto* Visual = Prop ? Prop->FindComponentByClass<UStaticMeshComponent>() : nullptr;
                        const UStaticMesh* Mesh = Visual ? Visual->GetStaticMesh() : nullptr;
                        Test.TestTrue(TEXT("A floating Completo is visible and passable"),
                            Mesh && Mesh->GetFName() == TEXT("SM_DCompleto") && Visual->IsVisible()
                            && !Prop->IsHidden() && Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
                        Test.TestTrue(TEXT("A floating Completo shares the player lane"),
                            FMath::IsNearlyEqual(It->GetActorLocation().Y, Position.Y, 1.));
                        ++Completos;
                    }
                    if (It->ActorHasTag(TEXT("Delerict.RouteEnd")))
                    {
                        EndX = It->GetComponentsBoundingBox().Min.X - Capsule->GetScaledCapsuleRadius();
                    }
                    if (It->ActorHasTag(TEXT("Delerict.RouteStart")))
                    {
                        StartWallX = It->GetComponentsBoundingBox().Max.X;
                    }
                }
                Test.TestEqual(TEXT("One Bodoque in the hangar"), Bodoques, 1);
                Test.TestEqual(TEXT("Seven floating Completos in the hangar"), Completos, 7);
                if (!Test.TestTrue(TEXT("Spawn is inside the start room, near its closed end"),
                    StartWallX.IsSet() && Position.X > StartWallX.GetValue() && Position.X - StartWallX.GetValue() < 800.)
                    || !Test.TestTrue(TEXT("The route continues beyond the room"), EndX.IsSet() && EndX.GetValue() > Position.X + 2000.))
                {
                    return true;
                }
                LaneY = Position.Y;
                if (FParse::Param(FCommandLine::Get(), TEXT("DelerictCaptureHangar")))
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("HangarStart.png"), false, false);
                }
                Pawn->QueueImpulse(FVector::ForwardVector * (500.f * Pawn->GetEffectiveMass()));
                bMoving = true;
            }
            else if (Position.X >= EndX.GetValue() - 5.)
            {
                Test.TestTrue(TEXT("The pawn reaches the far wall on the original lane"),
                    FMath::IsNearlyEqual(Position.X, EndX.GetValue(), 5.) && FMath::IsNearlyEqual(Position.Y, LaneY, 1.) && Mover->IsFlying());
                const auto& View = Player->PlayerCameraManager->GetCameraCacheView();
                Test.TestTrue(TEXT("The lateral camera follows the pawn"), View.Location.Y > Position.Y && FMath::IsNearlyEqual(View.Rotation.Yaw, -90., 1.));
                return true;
            }
            return false;
        }
    private:
        FAutomationTestBase& Test;
        const double Started = FPlatformTime::Seconds();
        TOptional<double> StartWallX;
        TOptional<double> EndX;
        double LaneY = 0.;
        bool bMoving = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDHangarRouteTest, "Delerict.Player.HangarRoute",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDHangarRouteTest::RunTest(const FString&)
{
    if (!AutomationOpenMap(TEXT("/Game/Maps/L_DHangar"))) { return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FDFlyHangar(*this));
    return true;
}

#endif
