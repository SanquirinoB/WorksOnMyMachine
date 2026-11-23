#pragma once

#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitData/CoreKitDataFragment.h"
#include "CoreKitSave/CoreKitWorldSave.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "CoreKitExampleActors.generated.h"

class UCoreKitExampleHeldIntent;
class UCoreKitActionComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class UCoreKitExampleInteraction : public UCoreKitDataFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Example")
	FText Label;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Example")
	float Duration = 0.45f;
};

USTRUCT()
struct FCoreKitExampleDoorState
{
	GENERATED_BODY()
	UPROPERTY()
	bool bOpen = false;
};

UCLASS()
class UCoreKitExampleDoorSave : public UCoreKitSaveParticipantComponent
{
	GENERATED_BODY()
public:
	bool CaptureSaveState_Implementation(FCoreKitParticipantState& OutState) override;
	bool CanRestoreSaveState_Implementation(const FCoreKitParticipantState& State) override;
	void ApplySaveState_Implementation(const FCoreKitParticipantState& State) override;
};

UCLASS(Blueprintable)
class ACoreKitExampleDoor : public AActor
{
	GENERATED_BODY()
public:
	ACoreKitExampleDoor();

	UFUNCTION(BlueprintCallable, Category = "Example")
	void SetOpen(bool bNewOpen, bool bAnimate = true);

	UFUNCTION(BlueprintPure, Category = "Example")
	bool IsOpen() const { return bOpen; }

protected:
	UPROPERTY(EditAnywhere, Category = "Example")
	float OpenAngle = 95.0f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<USceneComponent> Hinge;
	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UStaticMeshComponent> Panel;
	UPROPERTY(VisibleAnywhere, Category = "Example")
	TObjectPtr<UCoreKitExampleDoorSave> Persistence;
	bool bOpen = false;
};

UCLASS(Blueprintable)
class UCoreKitExampleBlockAction : public UCoreKitAction
{
	GENERATED_BODY()
};

UCLASS()
class ACoreKitExampleController : public APlayerController
{
	GENERATED_BODY()
public:
	ACoreKitExampleController();
	void SetupInputComponent() override;
	void BeginPlay() override;
	void SetBlockHeld(bool bHeld);
	void SimulateBusy();
	bool IsBlocking() const;

private:
	void ToggleDoor();
	void StartBlock();
	void StopBlock();
	UFUNCTION()
	void BlockStarted(FGameplayTag Tag, UCoreKitAction* Action);
	UFUNCTION()
	void BlockFinished(FGameplayTag Tag, UCoreKitAction* Action, ECoreKitActionEndReason Reason);
	FGuid BusyId;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCoreKitActionComponent> Actions;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCoreKitExampleHeldIntent> Held;
};

UCLASS()
class ACoreKitExampleGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ACoreKitExampleGameMode();
};
