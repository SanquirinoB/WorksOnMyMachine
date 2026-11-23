#pragma once

#include "GameFramework/Pawn.h"
#include "GenericPlatform/ICursor.h"
#include "MoverSimulationTypes.h"
#include "DPawn.generated.h"

class ADThrowable;
class USoundBase;
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
    bool IsAimingWithGamepad() const { return bGamepadAim; }
    FVector2D GetMoveIntent() const { return MoveIntent; }
    bool IsRunHeld() const { return bRunHeld; }
    void RequestInitialMagneticCapture() { bInitialMagneticCaptureRequested = true; }
    bool ConsumeMagneticCaptureRequest();
    bool HasMovementImpulse() const { return bHasMovementImpulse; }
    float GetEffectiveMass() const;
    int32 GetPlateCount() const { return PlateCount; }

    /** Number of owned world pickups, including deployed reusable gadgets. The starting plate stack is separate. */
    UFUNCTION(BlueprintPure, Category = "Player|Inventory")
    int32 GetPickedUpThrowableCount() const;

    /** Carried quantity for a tuning type key, including any remaining starting plates of that type. */
    UFUNCTION(BlueprintPure, Category = "Player|Inventory")
    int32 GetThrowableCountByType(FName ThrowableType) const;

    /** First world pickup currently carried, if any. */
    const ADThrowable* GetGrabbedThrowable() const;
    bool TryGrabThrowable(ADThrowable& Item);
    bool TryStowReusableThrowable(ADThrowable& Item);

    /** Selected type key, which can have a quantity of zero when selected via hotkey. */
    UFUNCTION(BlueprintPure, Category = "Player|Inventory")
    FName GetSelectedThrowableType() const;
    float GetThrowCharge() const;
    void PlayFoley(USoundBase* Sound) const;

    /** Queue a world-space impulse in kg*cm/s. A directed launch keeps only velocity along the impulse. */
    void QueueImpulse(const FVector& WorldImpulse, bool bAlignVelocity = false);

    FDChargeAction OnJumpCharge;
    FDChargeAction OnThrowCharge;
    FSimpleMulticastDelegate OnGrabRequested;
    FSimpleMulticastDelegate OnSelectThrowableRequested;

    /** Trigger the same death and retry flow as a room hazard. */
    UFUNCTION(BlueprintCallable, Exec, Category = "Player")
    void Die();

    void FellOutOfWorld(const UDamageType& DamageType) override;
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
    UPROPERTY(Transient) TObjectPtr<UInputAction> MagnetHoldAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> GrabAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> SelectThrowableAction;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UInputAction>> ThrowableHotkeyActions;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ThrowAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ActivateThrowableAction;
    UPROPERTY(Transient) TArray<TObjectPtr<ADThrowable>> GrabbedThrowables;
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
        bool bAlignVelocity;
    };
    TArray<FQueuedImpulse, TInlineAllocator<4>> PendingImpulses;
    struct FThrowCharge
    {
        double StartedAt;
        FName Type;
    };
    TOptional<FThrowCharge> PendingThrow;
    int32 PlateCount = 0;
    FName SelectedThrowableType;
    EMouseCursor::Type PreviousMouseCursor = EMouseCursor::Default;
    bool bPreviouslyShowedMouseCursor = false;
    bool bGamepadAim = false;
    bool bRunHeld = false;
    bool bBootsWereAttached = false;
    double LastBootAudioTime = 0.;
    double DistanceSinceFootstep = 0.;
    int32 NextBootStep = 0;
    void UpdateBootAudio();
    bool bMagnetHeld = false;
    bool bInitialMagneticCaptureRequested = false;
    bool bHasMovementImpulse = false;
    bool bJumpCharging = false;
    bool bThrowCharging = false;

    void CreateInputMappings();
    void ClearInput();
    void PauseGame();
    void GoToBodoqueRoom();
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
    void StartMagnetHold();
    void StopMagnetHold();
    void RequestGrab();
    void RequestThrowableSelection();
    void RequestThrowableTypeSelection(FName Type);
    void ActivateThrownItem();
    bool InteractWithNearbySwitch();
    void GrabNearbyThrowable();
    bool CanCarryThrowable(const ADThrowable& Item) const;
    void CycleSelectedThrowable();
    void SelectThrowableType(FName Type);
    void HandleThrowCharge(EDChargePhase Phase, const FVector& Aim);
    void LaunchSelectedThrowable(const FVector& Aim, float Charge);
    float GetThrowableMass(FName Type) const;
    float GetEffectiveMassWithout(const ADThrowable* OmittedItem) const;
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
