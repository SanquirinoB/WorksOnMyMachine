#pragma once

#include "CoreKitActions/Actions/CoreKitAction.h"
#include "CoreKitActions/Actions/CoreKitActionContext.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CoreKitActions/States/CoreKitStateProvider.h"
#include "UObject/Interface.h"
#include "CoreKitTestFixtures.generated.h"

UCLASS()
class ACoreKitTestOwner : public AActor
{
	GENERATED_BODY()

public:
	int32 Starts = 0;
	int32 Cleanups = 0;
	int32 CopiedValue = 0;
	int32 Ticks = 0;
	ECoreKitActionEndReason LastCleanupReason = ECoreKitActionEndReason::OwnerEndPlay;
};

UCLASS()
class UCoreKitTestAction : public UCoreKitAction
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 AuthoredValue = 0;

	UPROPERTY()
	bool bDelayCancellation = false;

	UPROPERTY()
	bool bAllowActivation = true;

	virtual bool EvaluateActivation_Implementation(FName& FailureCode, FString& Detail) const override;
	virtual void ReceiveCancelRequested_Implementation(ECoreKitActionEndReason Reason) override;

protected:
	virtual void Activate() override;
	virtual void CleanupAction(ECoreKitActionEndReason Reason) override;
};

/** Reentrant delegate fixture: completes a selected action from a tag notification. */
UCLASS()
class UCoreKitTestTagListener : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UCoreKitActionComponent> Actions = nullptr;

	UPROPERTY()
	FGameplayTag FinishWhenAdded;

	UPROPERTY()
	FGameplayTag ActionToFinish;

	UFUNCTION()
	void HandleTagAdded(FGameplayTag AddedTag);
};

UCLASS()
class UCoreKitTestTickAction : public UCoreKitTestAction
{
	GENERATED_BODY()

protected:
	virtual void TickTask(float DeltaTime) override;
};

/** Simulates a domain callback that completes the action by its own execution identity. */
UCLASS()
class UCoreKitTestAsyncAction : public UCoreKitTestAction
{
	GENERATED_BODY()

public:
	bool CompleteFromCallback(FGuid CallbackExecutionId);
};

UINTERFACE()
class UCoreKitTestMarkerInterface : public UInterface
{
	GENERATED_BODY()
};

class ICoreKitTestMarkerInterface
{
	GENERATED_BODY()
};

UCLASS()
class UCoreKitTestMarkerComponent : public UActorComponent, public ICoreKitTestMarkerInterface
{
	GENERATED_BODY()
};

UCLASS()
class UCoreKitTestContext : public UCoreKitActionContext
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bValid = true;

	virtual bool IsActionContextValid_Implementation() const override { return bValid; }
};

UCLASS()
class UCoreKitTestStateProvider : public UObject, public ICoreKitStateProvider
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGameplayTag CurrentState;

	virtual FGameplayTag QueryDomainState_Implementation(FName Domain) const override
	{
		return CurrentState;
	}
};
