#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CoreKitExampleHeldIntent.generated.h"

class UCoreKitActionComponent;
class UCoreKitAction;
enum class ECoreKitActionEndReason : uint8;

UCLASS(ClassGroup = (CoreKitExample), meta = (BlueprintSpawnableComponent))
class UCoreKitExampleHeldIntent : public UActorComponent
{
	GENERATED_BODY()

public:
	void Configure(UCoreKitActionComponent* InActions, FGameplayTag InActionTag);

	UFUNCTION(BlueprintCallable, Category = "Example")
	void SetHeld(bool bInHeld);

	UFUNCTION(BlueprintPure, Category = "Example")
	bool IsHeld() const { return bHeld; }

protected:
	void EndPlay(EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void ReadinessChanged(bool bReady);
	UFUNCTION()
	void ActionFinished(FGameplayTag Tag, UCoreKitAction* Action, ECoreKitActionEndReason Reason);
	UFUNCTION()
	void CoordinatorEnding();
	void TryStart();
	void ClearBinding();

	TWeakObjectPtr<UCoreKitActionComponent> Actions;
	FGameplayTag ActionTag;
	FGuid Execution;
	bool bHeld = false;
	bool bRequesting = false;
};
