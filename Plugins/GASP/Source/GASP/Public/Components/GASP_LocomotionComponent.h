#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Types/EnumTypes.h"
#include "GASP_LocomotionComponent.generated.h"

class UDataContainer;
class UDataDrivenComponent;
class UGASP_StateComponent;
class UGASP_CharacterMovementComponent;

UCLASS(ClassGroup=(GASP), meta=(BlueprintSpawnableComponent))
class GASP_API UGASP_LocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASP_LocomotionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "GASP|Locomotion")
	void ApplyDataDrivenSettings();

	UFUNCTION(BlueprintCallable, Category = "GASP|Locomotion")
	void RefreshLocomotionState();

	UFUNCTION(BlueprintCallable, Category = "GASP|Locomotion")
	void RefreshGait();

	UFUNCTION(BlueprintCallable, Category = "GASP|Locomotion")
	void RequestDesiredGait(const FGameplayTag NewGait, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "GASP|Locomotion")
	void RequestRotationMode(const FGameplayTag NewRotationMode, bool bForce = false);

	UFUNCTION(BlueprintPure, Category = "GASP|Locomotion")
	bool CanSprint() const;

	UFUNCTION(BlueprintPure, Category = "GASP|Locomotion")
	bool HasFullMovementInput() const;

	UFUNCTION(BlueprintPure, Category = "GASP|Locomotion")
	FVector2D GetMovementInputScaleValue(const FVector2D InVector) const;

	UFUNCTION(BlueprintPure, Category = "GASP|Locomotion")
	FORCEINLINE float GetAnalogMovementThreshold() const { return AnalogMovementThreshold; }

	UFUNCTION(BlueprintPure, Category = "GASP|Locomotion")
	FORCEINLINE EAnalogStickBehaviorMode GetMovementStickMode() const { return MovementStickMode; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Config")
	FGameplayTag SettingsContainerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogMovementThreshold{0.7f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion")
	EAnalogStickBehaviorMode MovementStickMode{EAnalogStickBehaviorMode::FixedSingleGait};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Locomotion", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SprintMaxDirectionAngle{50.0f};

	UPROPERTY(Transient)
	TObjectPtr<UGASP_StateComponent> StateComponent;

	UPROPERTY(Transient)
	TObjectPtr<UGASP_CharacterMovementComponent> MovementComponent;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestDesiredGait(const FGameplayTag NewGait, bool bForce);

	UFUNCTION(Server, Reliable)
	void ServerRequestRotationMode(const FGameplayTag NewRotationMode, bool bForce);

	UDataDrivenComponent* GetDataDrivenComponent() const;
	const UDataContainer* GetSettingsContainer() const;
};


