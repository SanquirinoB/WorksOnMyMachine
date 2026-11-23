#pragma once

#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"
#include "DPawn.generated.h"

class UAnimSequence;
class UCapsuleComponent;
class UCharacterMoverComponent;
class UDPlayerTuning;
class UEnhancedInputLocalPlayerSubsystem;
class UGameplayCameraComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS(Blueprintable)
class ADPawn : public APawn, public IMoverInputProducerInterface
{
    GENERATED_BODY()

public:
    ADPawn();
    const UDPlayerTuning* GetPlayerTuning() const;

    FVector GetVelocity() const override;
    void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmd) override;

protected:
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
    TObjectPtr<UCharacterMoverComponent> Mover;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UGameplayCameraComponent> GameplayCamera;

    /** Traversal tuning asset assigned in the playable pawn Blueprint. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UDPlayerTuning> PlayerTuning = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0", Units = "s"))
    float CameraLookAheadTime = .18f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "0", Units = "cm"))
    float CameraMaxLookAhead = 90.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1", Units = "cm/s"))
    float MaxGroundSpeed = 500.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1", Units = "cm/s"))
    float JumpSpeed = 550.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.1"))
    float GravityScale = .8f;

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
    UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
    float HorizontalIntent = 0.f;
    bool bJumpPressed = false;
    bool bJumpRequested = false;

    void CreateInputMappings();
    void ClearInput();
    void Move(const FInputActionValue& Value);
    void StopMoving();
    void StartJump();
    void StopJump();

    UFUNCTION()
    void UpdateCameraLead(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState);
};
