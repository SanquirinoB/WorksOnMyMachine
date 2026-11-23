#pragma once

#include "CoreMinimal.h"
#include "Actors/GASP_Character.h"
#include "Components/GASP_TraversalComponent.h"
#include "GASPGameplayCameraInterface.h"
#include "Types/EnumTypes.h"
#include "Types/StructTypes.h"
#include "GASP_PlayerCharacter.generated.h"

UCLASS()
class GASPEXTRAS_API AGASP_PlayerCharacter : public AGASP_Character, public IGASPGameplayCameraInterface
{
	GENERATED_BODY()

public:
	explicit AGASP_PlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void SprintAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void WalkAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void CrouchAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void JumpAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void AimAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void RagdollAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void StrafeAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	void MoveAction(const FVector2D& Value);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	void LookAction(const FVector2D& Value);

	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraversalResult TryTraversalAction() const;

	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsDoingTraversal() const;

	UFUNCTION(BlueprintPure, Category="GASP|Input")
	FVector2D GetMovementInputScaleValue(const FVector2D& InVector) const;

	virtual FCameraProperties GetCameraProperties_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess))
	TObjectPtr<class UGameplayCameraComponent> GameplayCamera{};

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AnalogMovementThreshold{.7f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	EAnalogStickBehaviorMode MovementStickMode{EAnalogStickBehaviorMode::FixedSingleGait};
};

