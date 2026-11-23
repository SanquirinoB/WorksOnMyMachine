#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GASP_StateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGASP_OnStateChanged, FGameplayTag, OldGameplayTag);

class UDataDrivenComponent;
class UDataContainer;

UCLASS(ClassGroup=(GASP), meta=(BlueprintSpawnableComponent))
class GASP_API UGASP_StateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASP_StateComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void ApplyDataDrivenDefaults();

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetDesiredGait(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetGait(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetRotationMode(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetMovementState(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetMovementMode(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetStanceMode(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetOverlayMode(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetPoseMode(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetLocomotionAction(const FGameplayTag NewValue, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|State")
	void SetReplicatedAcceleration(const FVector& NewAcceleration);

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetDesiredGait() const { return DesiredGait; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetGait() const { return Gait; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetRotationMode() const { return RotationMode; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetMovementState() const { return MovementState; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetMovementMode() const { return MovementMode; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetStanceMode() const { return StanceMode; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetOverlayMode() const { return OverlayMode; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetPoseMode() const { return PoseMode; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FGameplayTag GetLocomotionAction() const { return LocomotionAction; }

	UFUNCTION(BlueprintPure, Category = "GASP|State")
	FORCEINLINE FVector GetReplicatedAcceleration() const { return ReplicatedAcceleration; }

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged DesiredGaitChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged GaitChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged RotationModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged MovementStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged MovementModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged StanceModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged OverlayModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged PoseModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "GASP|State")
	FGASP_OnStateChanged LocomotionActionChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Config")
	FGameplayTag DefaultsContainerTag;

	UPROPERTY(ReplicatedUsing = OnRep_DesiredGait, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag DesiredGait;

	UPROPERTY(BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag Gait;

	UPROPERTY(ReplicatedUsing = OnRep_RotationMode, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag RotationMode;

	UPROPERTY(ReplicatedUsing = OnRep_MovementState, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag MovementState;

	UPROPERTY(ReplicatedUsing = OnRep_MovementMode, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag MovementMode;

	UPROPERTY(ReplicatedUsing = OnRep_StanceMode, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag StanceMode;

	UPROPERTY(ReplicatedUsing = OnRep_OverlayMode, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag OverlayMode;

	UPROPERTY(ReplicatedUsing = OnRep_PoseMode, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag PoseMode;

	UPROPERTY(ReplicatedUsing = OnRep_LocomotionAction, BlueprintReadOnly, Category = "GASP|State")
	FGameplayTag LocomotionAction;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "GASP|State")
	FVector_NetQuantize ReplicatedAcceleration{ForceInit};

	UFUNCTION()
	void OnRep_DesiredGait(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_RotationMode(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_MovementState(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_MovementMode(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_StanceMode(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_OverlayMode(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_PoseMode(const FGameplayTag& OldValue);

	UFUNCTION()
	void OnRep_LocomotionAction(const FGameplayTag& OldValue);

private:
	template<typename DelegateType>
	void BroadcastTagChange(DelegateType& Delegate, const FGameplayTag& OldValue) const
	{
		Delegate.Broadcast(OldValue);
	}

	UDataDrivenComponent* GetDataDrivenComponent() const;
	const UDataContainer* GetDefaultsContainer() const;
};


