#include "DPawn.h"
#include "../Frontend/DMenu.h"

#include "DMoverComponent.h"
#include "DPlayerTuning.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "MoveLibrary/ConstrainedMoveUtils.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG
#include "../Debug/DGameplayDebug.h"
#endif

namespace
{
    bool ReadViewportMousePosition(const APlayerController& Player, FVector2D& Position)
    {
        const ULocalPlayer* LocalPlayer = Player.GetLocalPlayer();
        const UGameViewportClient* ViewportClient = LocalPlayer ? LocalPlayer->ViewportClient : nullptr;
        const FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
        if (!Viewport || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().IsActive())
        {
            return false;
        }

        const TSharedPtr<SViewport> Widget = ViewportClient->GetGameViewportWidget();
        if (!Widget)
        {
            return false;
        }

        // The viewport's last pointer event can go stale during capture or focus changes.
        // Read the live Slate cursor and convert its desktop position to render pixels.
        const FGeometry& Geometry = Widget->GetTickSpaceGeometry();
        const FVector2D LocalSize = Geometry.GetLocalSize();
        const FIntPoint PixelSize = Viewport->GetSizeXY();
        if (LocalSize.X <= 0. || LocalSize.Y <= 0. || PixelSize.X <= 0 || PixelSize.Y <= 0.)
        {
            return false;
        }
        const FVector2D LocalMouse = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
        if (LocalMouse.X < 0. || LocalMouse.Y < 0. || LocalMouse.X >= LocalSize.X || LocalMouse.Y >= LocalSize.Y)
        {
            return false;
        }
        Position = LocalMouse / LocalSize * FVector2D(PixelSize);
        return true;
    }
}

void ADPawn::CreateInputMappings()
{
    const UDPlayerTuning& Tuning = GetTuning();
    InputContext = NewObject<UInputMappingContext>(this);
    MoveAction = NewObject<UInputAction>(this);
    MoveAction->ValueType = EInputActionValueType::Axis2D;
    MoveAction->AccumulationBehavior = EInputActionAccumulationBehavior::Cumulative;
    AimAction = NewObject<UInputAction>(this);
    AimAction->ValueType = EInputActionValueType::Axis2D;
    RunAction = NewObject<UInputAction>(this);
    MagnetHoldAction = NewObject<UInputAction>(this);
    JumpAction = NewObject<UInputAction>(this);
    GrabAction = NewObject<UInputAction>(this);
    SelectThrowableAction = NewObject<UInputAction>(this);
    ThrowAction = NewObject<UInputAction>(this);
    ActivateThrowableAction = NewObject<UInputAction>(this);

    const auto MapDirection = [this](const TArray<FKey>& Keys, bool bNegative, bool bVertical)
    {
        for (const FKey& Key : Keys)
        {
            FEnhancedActionKeyMapping& Mapping = InputContext->MapKey(MoveAction, Key);
            if (bNegative)
            {
                Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
            }
            if (bVertical)
            {
                auto* Swizzle = NewObject<UInputModifierSwizzleAxis>(InputContext);
                Swizzle->Order = EInputAxisSwizzle::YXZ;
                Mapping.Modifiers.Add(Swizzle);
            }
        }
    };
    MapDirection(Tuning.MoveLeftKeys, true, false);
    MapDirection(Tuning.MoveRightKeys, false, false);
    MapDirection(Tuning.MoveUpKeys, false, true);
    MapDirection(Tuning.MoveDownKeys, true, true);

    if (Tuning.MoveStickDeadZone < 1.f)
    {
        auto* DeadZone = NewObject<UInputModifierDeadZone>(InputContext);
        DeadZone->LowerThreshold = FMath::Max(Tuning.MoveStickDeadZone, 0.f);
        InputContext->MapKey(MoveAction, Tuning.MoveStickKey).Modifiers.Add(DeadZone);
    }
    InputContext->MapKey(AimAction, Tuning.AimStickKey);

    const auto MapButtons = [this](UInputAction* Action, const TArray<FKey>& Keys)
    {
        for (const FKey& Key : Keys)
        {
            InputContext->MapKey(Action, Key);
        }
    };
    MapButtons(RunAction, Tuning.RunKeys);
    MapButtons(MagnetHoldAction, Tuning.MagnetHoldKeys);
    MapButtons(JumpAction, Tuning.JumpKeys);
    MapButtons(GrabAction, Tuning.GrabKeys);
    MapButtons(SelectThrowableAction, Tuning.SelectThrowableKeys);
    for (const auto& Binding : Tuning.ThrowableHotkeys)
    {
        if (!Binding.Key.IsNone() && Binding.Value.IsValid())
        {
            UInputAction* Action = NewObject<UInputAction>(this);
            ThrowableHotkeyActions.Add(Binding.Key, Action);
            InputContext->MapKey(Action, Binding.Value);
        }
    }
    MapButtons(ThrowAction, Tuning.ThrowKeys);
    MapButtons(ActivateThrowableAction, Tuning.ActivateThrowableKeys);
}

void ADPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    auto* Player = Cast<APlayerController>(GetController());
    if (!Input || !Player || !Player->GetLocalPlayer())
    {
        return;
    }

    ClearInput();
    SetUpAimCursor(*Player);
    if (!InputContext)
    {
        CreateInputMappings();
    }
    InputSubsystem = Player->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (InputSubsystem.IsValid())
    {
        InputSubsystem->AddMappingContext(InputContext, 0);
    }

    Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
    Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopMoving);
    Input->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::StopMoving);
    Input->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::AimStick);
    Input->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::ClearAimStick);
    Input->BindAction(AimAction, ETriggerEvent::Canceled, this, &ThisClass::ClearAimStick);
    Input->BindAction(RunAction, ETriggerEvent::Started, this, &ThisClass::StartRun);
    Input->BindAction(RunAction, ETriggerEvent::Completed, this, &ThisClass::StopRun);
    Input->BindAction(RunAction, ETriggerEvent::Canceled, this, &ThisClass::StopRun);
    Input->BindAction(MagnetHoldAction, ETriggerEvent::Started, this, &ThisClass::StartMagnetHold);
    Input->BindAction(MagnetHoldAction, ETriggerEvent::Completed, this, &ThisClass::StopMagnetHold);
    Input->BindAction(MagnetHoldAction, ETriggerEvent::Canceled, this, &ThisClass::StopMagnetHold);
    Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJumpCharge);
    Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::ReleaseJumpCharge);
    Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::CancelJumpCharge);
    Input->BindAction(GrabAction, ETriggerEvent::Started, this, &ThisClass::RequestGrab);
    Input->BindAction(SelectThrowableAction, ETriggerEvent::Started, this, &ThisClass::RequestThrowableSelection);
    for (const auto& Binding : ThrowableHotkeyActions)
    {
        Input->BindAction(Binding.Value, ETriggerEvent::Started, this, &ThisClass::RequestThrowableTypeSelection, Binding.Key);
    }
    Input->BindAction(ThrowAction, ETriggerEvent::Started, this, &ThisClass::StartThrowCharge);
    Input->BindAction(ThrowAction, ETriggerEvent::Completed, this, &ThisClass::ReleaseThrowCharge);
    Input->BindAction(ThrowAction, ETriggerEvent::Canceled, this, &ThisClass::CancelThrowCharge);
    Input->BindAction(ActivateThrowableAction, ETriggerEvent::Started, this, &ThisClass::ActivateThrownItem);
    PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &ThisClass::GoToBodoqueRoom);
    PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::PauseGame);
    PlayerInputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &ThisClass::PauseGame);
}

void ADPawn::GoToBodoqueRoom()
{
    if (!HasAuthority())
    {
        return;
    }

    TArray<AActor*> Arrivals;
    UGameplayStatics::GetAllActorsWithTag(this, TEXT("Delerict.BodoqueRoomStart"), Arrivals);
    if (Arrivals.Num() != 1)
    {
        return;
    }

    StopMoving();
    StopRun();
    StopMagnetHold();
    CancelJumpCharge();
    CancelThrowCharge();
    bInitialMagneticCaptureRequested = false;
    PendingImpulses.Reset();

    auto Teleport = MakeShared<FTeleportEffect>();
    Teleport->TargetLocation = UPlanarConstraintUtils::ConstrainLocationToPlane(
        Mover->GetPlanarConstraint(), Arrivals[0]->GetActorLocation());
    Teleport->bUseActorRotation = false;
    Teleport->TargetRotation = GetTuning().LockedRotation;
    Mover->QueueInstantMovementEffect(Teleport);

    auto Stop = MakeShared<FApplyVelocityEffect>();
    Stop->ForceMovementMode = DefaultModeNames::Flying;
    Mover->QueueInstantMovementEffect(Stop);
}

void ADPawn::PauseGame()
{
    if (auto* Player = Cast<APlayerController>(GetController()); Player && UDMenu::OpenPause(*Player))
    {
        StopMoving();
        StopRun();
        StopMagnetHold();
        ClearAimStick();
        CancelJumpCharge();
        CancelThrowCharge();
    }
}

void ADPawn::ClearInput()
{
    if (InputSubsystem.IsValid() && InputContext)
    {
        InputSubsystem->RemoveMappingContext(InputContext);
    }
    if (auto* Input = Cast<UEnhancedInputComponent>(InputComponent))
    {
        Input->ClearBindingsForObject(this);
    }
    InputSubsystem.Reset();
    StopMoving();
    StopRun();
    StopMagnetHold();
    ClearAimStick();
    CancelJumpCharge();
    CancelThrowCharge();
}

void ADPawn::SetUpAimCursor(APlayerController& Player)
{
    if (AimController.Get() == &Player)
    {
        return;
    }
    RestoreAimCursor();
    AimController = &Player;
    bPreviouslyShowedMouseCursor = Player.bShowMouseCursor;
    PreviousMouseCursor = Player.CurrentMouseCursor;
    Player.SetShowMouseCursor(true);
    Player.CurrentMouseCursor = EMouseCursor::Crosshairs;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockInFullscreen);
    Player.SetInputMode(InputMode);
}

void ADPawn::RestoreAimCursor()
{
    if (APlayerController* Player = AimController.Get())
    {
        Player->bShowMouseCursor = bPreviouslyShowedMouseCursor;
        Player->CurrentMouseCursor = PreviousMouseCursor;
        Player->SetInputMode(FInputModeGameOnly());
    }
    AimController.Reset();
    LastMousePosition.Reset();
    bGamepadAim = false;
}

bool ADPawn::SetAimDirection(const FVector& Direction)
{
    const FVector InPlane = UPlanarConstraintUtils::ConstrainDirectionToPlane(
        Mover->GetPlanarConstraint(), Direction).GetSafeNormal();
    if (InPlane.ContainsNaN() || InPlane.IsNearlyZero())
    {
        return false;
    }
    AimDirection = InPlane;
    return true;
}

void ADPawn::UpdateAim()
{
    APlayerController* Player = AimController.Get();
    if (!Player)
    {
        return;
    }

    FVector2D Mouse;
    const bool bHasMouse = ReadViewportMousePosition(*Player, Mouse);
    if (bHasMouse)
    {
        if (LastMousePosition && !LastMousePosition->Equals(Mouse, .5))
        {
            bGamepadAim = false;
        }
        LastMousePosition = Mouse;
    }

    const FVector Location = GetActorLocation();
    FVector RayStart;
    FVector RayDirection;
    TOptional<FVector> Intersection;
    const bool bHasRay = !bGamepadAim && LastMousePosition
        && Player->DeprojectScreenPositionToWorld(LastMousePosition->X, LastMousePosition->Y, RayStart, RayDirection);
    if (bHasRay)
    {
        const FPlanarConstraint& Plane = Mover->GetPlanarConstraint();
        const double RayDotNormal = FVector::DotProduct(RayDirection, Plane.PlaneConstraintNormal);
        if (!FMath::IsNearlyZero(RayDotNormal))
        {
            const double Distance = FVector::DotProduct(Plane.PlaneConstraintOrigin - RayStart,
                Plane.PlaneConstraintNormal) / RayDotNormal;
            if (Distance >= 0.)
            {
                Intersection = RayStart + RayDirection * Distance;
                SetAimDirection(*Intersection - Location);
            }
        }
    }

#if DELERICT_WITH_GAMEPLAY_DEBUG
    using namespace DGameplayDebug;
    UWorld* World = GetWorld();
    if (IsEnabled(World, EChannel::Aim))
    {
        if (bHasRay)
        {
            DrawLine(World, EChannel::Aim, RayStart, Intersection.Get(RayStart + RayDirection * 450.));
        }
        if (Intersection)
        {
            DrawLine(World, EChannel::Aim, *Intersection - FVector(0., 0., 8.), *Intersection + FVector(0., 0., 8.));
        }
        DrawArrow(World, EChannel::Aim, Location, Location + AimDirection * 220.);
        const TCHAR* Source = bGamepadAim ? TEXT("Right stick") : (Intersection ? TEXT("Mouse") : TEXT("Last aim"));
        DrawLabel(World, EChannel::Aim, Location + FVector(0., 0., 125.),
            FString::Printf(TEXT("%s | direction %s"), Source, *AimDirection.ToCompactString()));
    }
#endif
}

bool ADPawn::GetCrosshairPosition(FVector2D& Position) const
{
    const APlayerController* Player = AimController.Get();
    if (!Player)
    {
        return false;
    }
    if (!bGamepadAim)
    {
        if (ReadViewportMousePosition(*Player, Position))
        {
            return true;
        }
        if (LastMousePosition)
        {
            Position = *LastMousePosition;
            return true;
        }
    }
    return Player->ProjectWorldLocationToScreen(GetActorLocation() + AimDirection * 240., Position, true);
}

void ADPawn::Move(const FInputActionValue& Value)
{
    MoveIntent = Value.Get<FVector2D>();
    if (MoveIntent.SizeSquared() > 1.)
    {
        MoveIntent.Normalize();
    }
}

void ADPawn::AimStick(const FInputActionValue& Value)
{
    const FVector2D Stick = Value.Get<FVector2D>();
    if (LastAimStick.Equals(Stick, .01))
    {
        return;
    }
    LastAimStick = Stick;

    const double DeadZone = FMath::Clamp(GetTuning().AimStickDeadZone, 0.f, 1.f);
    if (DeadZone < 1. && Stick.SizeSquared() > FMath::Square(DeadZone)
        && SetAimDirection(FVector(Stick.X, 0., Stick.Y)))
    {
        bGamepadAim = true;
    }
}

void ADPawn::StopMoving() { MoveIntent = FVector2D::ZeroVector; }
void ADPawn::ClearAimStick() { LastAimStick = FVector2D::ZeroVector; }
void ADPawn::StartRun() { bRunHeld = true; }
void ADPawn::StopRun() { bRunHeld = false; }
void ADPawn::StartMagnetHold() { bMagnetHeld = true; }
void ADPawn::StopMagnetHold() { bMagnetHeld = false; }

void ADPawn::RequestGrab()
{
    if (!InteractWithNearbySwitch())
    {
        GrabNearbyThrowable();
    }
    OnGrabRequested.Broadcast();
}

void ADPawn::RequestThrowableSelection()
{
    CycleSelectedThrowable();
    OnSelectThrowableRequested.Broadcast();
}

void ADPawn::RequestThrowableTypeSelection(FName Type)
{
    SelectThrowableType(Type);
    OnSelectThrowableRequested.Broadcast();
}

void ADPawn::UpdateCharge(bool& bCharging, FDChargeAction& Action, EDChargePhase Phase)
{
    const bool bPressed = Phase == EDChargePhase::Pressed;
    if (bPressed == bCharging)
    {
        return;
    }
    bCharging = bPressed;
    if (Phase == EDChargePhase::Released)
    {
        UpdateAim();
    }
    Action.Broadcast(Phase, AimDirection);
}

void ADPawn::StartJumpCharge() { UpdateCharge(bJumpCharging, OnJumpCharge, EDChargePhase::Pressed); }
void ADPawn::ReleaseJumpCharge() { UpdateCharge(bJumpCharging, OnJumpCharge, EDChargePhase::Released); }
void ADPawn::CancelJumpCharge() { UpdateCharge(bJumpCharging, OnJumpCharge, EDChargePhase::Canceled); }
void ADPawn::StartThrowCharge() { UpdateCharge(bThrowCharging, OnThrowCharge, EDChargePhase::Pressed); }
void ADPawn::ReleaseThrowCharge() { UpdateCharge(bThrowCharging, OnThrowCharge, EDChargePhase::Released); }
void ADPawn::CancelThrowCharge() { UpdateCharge(bThrowCharging, OnThrowCharge, EDChargePhase::Canceled); }
