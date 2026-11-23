#pragma once

#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"
#include "WorksOnMyMachinePawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UCharacterMoverComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS(Blueprintable)
class AWorksOnMyMachinePawn : public APawn, public IMoverInputProducerInterface
{
    GENERATED_BODY()

public:
    AWorksOnMyMachinePawn();

    FVector GetVelocity() const override;
    void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmd) override;

protected:
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type Reason) override;
    void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void UnPossessed() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UCapsuleComponent> Capsule;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
    TObjectPtr<UCharacterMoverComponent> Mover;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1", Units = "cm/s"))
    float MaxGroundSpeed = 500.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1", Units = "cm/s"))
    float JumpSpeed = 550.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.1"))
    float GravityScale = .8f;

private:
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
    void UpdateCameraPosition(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState);
};
