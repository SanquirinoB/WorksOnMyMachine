#pragma once

#include "GameFramework/Pawn.h"
#include "GenericPlatform/ICursor.h"
#include "MoverSimulationTypes.h"
#include "DPawn.generated.h"

class ADThrowable;
class UAnimSequence;
class UCapsuleComponent;
class UDMoverComponent;
class UDPlayerTuning;
class UEnhancedInputLocalPlayerSubsystem;
class UGameplayCameraComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;
class UStaticMeshComponent;
struct FInputActionValue;

enum class EDChargePhase : uint8 { Pressed, Released, Canceled };
DECLARE_MULTICAST_DELEGATE_TwoParams(FDChargeAction, EDChargePhase, const FVector&);

UCLASS(Blueprintable)
class ADPawn : public APawn, public IMoverInputProducerInterface
{
    GENERATED_BODY()

public:
    ADPawn();
    const UDPlayerTuning* GetPlayerTuning() const;
    const UDPlayerTuning& GetTuning() const;

    FVector GetAimDirection() const { return AimDirection; }
    bool GetCrosshairPosition(FVector2D& Position) const;
    FVector2D GetMoveIntent() const { return MoveIntent; }
    bool IsRunHeld() const { return bRunHeld; }
    float GetEffectiveMass() const;
    int32 GetPlateCount() const { return PlateCount; }
    const ADThrowable* GetGrabbedThrowable() const;
    FName GetSelectedThrowableType() const;
    float GetThrowCharge() const;

    /** Queue a world-space impulse in kg*cm/s using the effective mass at the time of the event. */
    void QueueImpulse(const FVector& WorldImpulse);

    FDChargeAction OnJumpCharge;
    FDChargeAction OnThrowCharge;
    FSimpleMulticastDelegate OnGrabRequested;
    FSimpleMulticastDelegate OnSelectThrowableRequested;

    FVector GetVelocity() const override;
    void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmd) override;

protected:
    void PreInitializeComponents() override;
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;
    void PossessedBy(AController* NewController) override;
    void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void UnPossessed() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UCapsuleComponent> Capsule;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<USkeletalMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UDMoverComponent> Mover;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UGameplayCameraComponent> GameplayCamera;

    /** Traversal tuning asset assigned in the playable pawn Blueprint. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UDPlayerTuning> PlayerTuning = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimSequence> IdleAnimation;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimSequence> WalkAnimation;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimSequence> RunAnimation;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    TObjectPtr<UAnimSequence> AirborneAnimation;

private:
    friend struct FDLocomotionProxy;

    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> InputContext;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AimAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> RunAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> GrabAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> SelectThrowableAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ThrowAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ActivateThrowableAction;
    UPROPERTY(Transient) TObjectPtr<ADThrowable> GrabbedThrowable;
    TWeakObjectPtr<ADThrowable> RemoteThrowable;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
    TWeakObjectPtr<APlayerController> AimController;
    TOptional<FVector2D> LastMousePosition;
    FVector2D LastAimStick = FVector2D::ZeroVector;
    FVector2D MoveIntent = FVector2D::ZeroVector;
    FVector AimDirection = FVector::ForwardVector;
    struct FQueuedImpulse
    {
        FVector WorldImpulse;
        float Mass;
    };
    TArray<FQueuedImpulse, TInlineAllocator<4>> PendingImpulses;
    TOptional<double> ThrowChargeStartedAt;
    int32 PlateCount = 0;
    bool bGrabbedThrowableSelected = false;
    EMouseCursor::Type PreviousMouseCursor = EMouseCursor::Default;
    bool bPreviouslyShowedMouseCursor = false;
    bool bGamepadAim = false;
    bool bRunHeld = false;
    bool bJumpCharging = false;
    bool bThrowCharging = false;

    void CreateInputMappings();
    void ClearInput();
    void PauseGame();
    void SetUpAimCursor(APlayerController& Player);
    void RestoreAimCursor();
    void UpdateAim();
    bool SetAimDirection(const FVector& Direction);
    void Move(const FInputActionValue& Value);
    void StopMoving();
    void AimStick(const FInputActionValue& Value);
    void ClearAimStick();
    void StartRun();
    void StopRun();
    void RequestGrab();
    void RequestThrowableSelection();
    void ActivateThrownItem();
    void GrabNearbyThrowable();
    void CycleSelectedThrowable();
    void HandleThrowCharge(EDChargePhase Phase, const FVector& Aim);
    void LaunchSelectedThrowable(const FVector& Aim, float Charge);
    float GetThrowableMass(FName Type) const;
    FVector GetQueuedVelocityChange(FVector* TotalImpulse = nullptr) const;
    void UpdateCharge(bool& bCharging, FDChargeAction& Action, EDChargePhase Phase);
    void StartJumpCharge();
    void ReleaseJumpCharge();
    void CancelJumpCharge();
    void StartThrowCharge();
    void ReleaseThrowCharge();
    void CancelThrowCharge();

    UFUNCTION()
    void UpdateCameraLead(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState);

#if DELERICT_WITH_GAMEPLAY_DEBUG
    FVector DebugImpulse = FVector::ZeroVector;
    FVector DebugVelocityChange = FVector::ZeroVector;
    FVector DebugSweepStart = FVector::ZeroVector;
    FVector DebugItemVelocity = FVector::ZeroVector;
    FVector DebugThrowRecoil = FVector::ZeroVector;
    FString DebugThrowResult;
    void RecordThrowResult(const TCHAR* Result, const FVector& ItemVelocity = FVector::ZeroVector,
        const FVector& Recoil = FVector::ZeroVector);
    void DrawThrowDiagnostics() const;
#endif
};
