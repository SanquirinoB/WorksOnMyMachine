#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types/StructTypes.h"
#include "GASP_Character.generated.h"

class UDataDrivenComponent;
class UMotionWarpingComponent;
class UGASP_StateComponent;
class UGASP_LocomotionComponent;
class UGASP_AnimationBindingComponent;
class UGASP_RagdollComponent;
class UGASP_TraversalComponent;

UCLASS()
class GASP_API AGASP_Character : public ACharacter
{
	GENERATED_BODY()

public:
	explicit AGASP_Character(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UGASP_StateComponent* GetStateComponent() const { return StateComponent; }

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UGASP_LocomotionComponent* GetLocomotionComponent() const { return LocomotionComponent; }

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UGASP_TraversalComponent* GetTraversalComponent() const { return TraversalComponent; }

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UGASP_AnimationBindingComponent* GetAnimationBindingComponent() const { return AnimationBindingComponent; }

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UGASP_RagdollComponent* GetRagdollComponent() const { return RagdollComponent; }

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintPure, Category = "GASP|Components")
	FORCEINLINE UDataDrivenComponent* GetDataDrivenComponent() const { return DataDrivenComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UDataDrivenComponent> DataDrivenComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UGASP_StateComponent> StateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UGASP_LocomotionComponent> LocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UGASP_AnimationBindingComponent> AnimationBindingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UGASP_RagdollComponent> RagdollComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP|Components")
	TObjectPtr<UGASP_TraversalComponent> TraversalComponent;
};


