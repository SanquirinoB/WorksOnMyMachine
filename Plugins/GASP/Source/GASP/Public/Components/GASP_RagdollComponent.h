#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Types/StructTypes.h"
#include "GASP_RagdollComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UCapsuleComponent;
class UGASP_StateComponent;
class UGASP_CharacterMovementComponent;
class USkeletalMeshComponent;
class UDataContainer;
class UDataDrivenComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGASP_RagdollEvent);

UCLASS(ClassGroup=(GASP), meta=(BlueprintSpawnableComponent))
class GASP_API UGASP_RagdollComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASP_RagdollComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	bool IsRagdollingAllowedToStart() const;

	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	bool IsRagdollingAllowedToStop() const;

	UFUNCTION(BlueprintCallable, Category = "GASP|Ragdoll")
	void StartRagdolling();

	UFUNCTION(BlueprintCallable, Category = "GASP|Ragdoll", Meta = (ReturnDisplayName = "Success"))
	bool StopRagdolling();

	UFUNCTION(BlueprintCallable, Category = "GASP|Ragdoll")
	void StartRagdollingImplementation();

	UFUNCTION(BlueprintCallable, Category = "GASP|Ragdoll", Meta = (ReturnDisplayName = "Success"))
	bool StopRagdollingImplementation();

	UFUNCTION(BlueprintCallable, Category = "GASP|Ragdoll")
	void SetRagdollTargetLocation(const FVector& NewTargetLocation);

	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	const FRagdollingState& GetRagdollingState() const;

	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	UAnimMontage* SelectGetUpMontage(bool bRagdollFacingUpward) const;

	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	float GetFlailPlayRateReferenceSpeed() const { return FlailPlayRateReferenceSpeed; }

	UPROPERTY(BlueprintAssignable, Category = "GASP|Ragdoll")
	FGASP_RagdollEvent OnRagdollStarted;

	UPROPERTY(BlueprintAssignable, Category = "GASP|Ragdoll")
	FGASP_RagdollEvent OnRagdollStopped;

	UFUNCTION(BlueprintImplementableEvent, Category = "GASP|Ragdoll", DisplayName = "On Ragdoll Started")
	void ReceiveRagdollStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "GASP|Ragdoll", DisplayName = "On Ragdoll Stopped")
	void ReceiveRagdollStopped();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Config")
	FGameplayTag SettingsContainerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll")
	TObjectPtr<UAnimMontage> GetUpMontageFront{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll")
	TObjectPtr<UAnimMontage> GetUpMontageBack{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll")
	uint8 bLimitInitialRagdollSpeed : 1{false};

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "GASP|Ragdoll")
	FVector_NetQuantize RagdollTargetLocation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", Transient)
	FRagdollingState RagdollingState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float MontageBlendOutDuration{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float MinInitialSpeedLimit{200.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float PullForce{750.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float PullForceInterpolationSpeed{0.6f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float PullForceMinDistance{5.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float PullForceMaxDistance{50.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float SpinePullSpeedThreshold{300.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float MotorReferenceSpeed{1000.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float MotorStiffness{25000.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Ragdoll", meta = (ClampMin = "0.0"))
	float FlailPlayRateReferenceSpeed{1000.0f};

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling();

	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void CacheOwnerDependencies();
	void ApplyDataDrivenSettings();
	void RefreshRagdolling(float DeltaTime);
	FVector RagdollTraceGround(bool& bGrounded) const;
	void ConstraintRagdollSpeed() const;
	UDataDrivenComponent* GetDataDrivenComponent() const;
	const UDataContainer* GetSettingsContainer() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> CharacterOwner{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UGASP_StateComponent> StateComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UGASP_CharacterMovementComponent> MovementComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> MeshComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UCapsuleComponent> CapsuleComponent{};
};


