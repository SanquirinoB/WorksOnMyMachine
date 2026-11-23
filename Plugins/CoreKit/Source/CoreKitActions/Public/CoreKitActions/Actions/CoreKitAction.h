#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTask.h"
#include "CoreKitActions/Actions/Types/CoreKitActionTypes.h"
#include "CoreKitActions/Actions/Types/CoreKitActorComponentContext.h"
#include "CoreKitAction.generated.h"

class UCoreKitActionComponent;
class UCoreKitActionContext;
class UCoreKitActionDefinition;
class UActorComponent;
class UAnimInstance;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoreKitActionSimpleEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitActionEndedEvent, ECoreKitActionEndReason, Reason);

/**
 * Runtime action implemented as a GameplayTask.
 * Subclass in C++ or Blueprint and call CompleteAction when its work is done.
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, Category = "CoreKit Actions Actions")
class COREKITACTIONS_API UCoreKitAction : public UGameplayTask
{
	GENERATED_BODY()

public:
	explicit UCoreKitAction(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionSimpleEvent OnActionActivated;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionEndedEvent OnActionEnded;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FActionFinishedNative, UCoreKitAction*, ECoreKitActionEndReason);
	FActionFinishedNative OnActionFinishedNative;

	static UCoreKitAction* CreateAction(
		IGameplayTaskOwnerInterface& TaskOwner,
		UObject* Outer,
		UClass* ActionClass,
		UCoreKitAction* ActionTemplate = nullptr
	);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void CompleteAction();

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void CancelAction();

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void FailAction();

	/** Graceful cancellation may wait for backend cleanup. Force is reserved for teardown. */
	bool RequestCancellation(ECoreKitActionEndReason Reason, bool bForce = false);

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	FGuid GetExecutionId() const { return ExecutionId; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	bool IsEnding() const { return bCancelRequested || bFinishRequested; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	FName GetActionPhase() const { return ActionPhase; }

	ECoreKitActionEndReason GetPendingEndReason() const { return PendingEndReason; }

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void SetActionPhase(FName Phase);

	/** Sets this execution's readiness contribution; true cannot release another state's block. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA")
	void SetIASA(bool bReady);

	/** Binds the generic IASA notify to this exact playback, never just its montage asset. */
	bool BindIASAMontage(UAnimInstance* Animation, int32 MontageInstanceId);
	/** Blueprint convenience after starting playback; captures the current instance immediately. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA")
	bool BindIASAAnimation(UAnimInstance* Animation, UAnimMontage* Montage);

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	FGameplayTag GetActionTag() const { return ActionTag; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	FGameplayTag GetActivationInputTag() const { return ActivationInputTag; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	float GetElapsedTime() const { return ElapsedTime; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	AActor* GetActionOwner() const { return OwnerActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	UCoreKitActionComponent* GetActionComponent() const { return ActionComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	UCoreKitActionDefinition* GetActionDefinition() const { return Definition.Get(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	UCoreKitActionContext* GetActionContext() const { return Context; }

	/** Resolves a component from the owner's cached context without searching the actor. */
	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actor Context",
		meta = (DeterminesOutputType = "ComponentClass"))
	UActorComponent* GetCachedOwnerComponentByClass(TSubclassOf<UActorComponent> ComponentClass,
		ECoreKitActorComponentLookupResult& Result, FName ComponentName = NAME_None) const;

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Tags")
	void AddOwnedTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Tags")
	void RemoveOwnedTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Tags")
	bool HasActiveTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Tags")
	bool HasAllActiveTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "CoreKit Actions|Actions")
	bool CanActivateAction() const;

	/** Query only: implementations must not change gameplay state. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit Actions|Actions")
	bool EvaluateActivation(FName& FailureCode, FString& Detail) const;

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "CoreKit Actions|Actions")
	bool CanCancelAction() const;

	/** Override for an asynchronous close, then call FinishCancellation when cleanup ends. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit Actions|Actions")
	void ReceiveCancelRequested(ECoreKitActionEndReason Reason);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void FinishCancellation();

	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit Actions|Actions",
		meta = (DisplayName = "Activate Action"))
	void ReceiveActivateAction();

	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit Actions|Actions", meta = (DisplayName = "Tick Action"))
	void ReceiveTickAction(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit Actions|Actions", meta = (DisplayName = "End Action"))
	void ReceiveEndAction(ECoreKitActionEndReason Reason);

	void InitializeAction(
		UCoreKitActionComponent* InComponent,
		FGameplayTag InActionTag,
		FGameplayTag InActivationInputTag,
		UCoreKitActionDefinition* InDefinition,
		UCoreKitActionContext* InContext = nullptr
	);
	void FinishAction(ECoreKitActionEndReason Reason);

protected:
	/** Optional domain recovery after this execution reaches its global IASA point. */
	virtual void OnIASAReleased() {}

	/** Runs once under the coordinator's teardown guard, before tags/delegates are released. */
	virtual void CleanupAction(ECoreKitActionEndReason Reason)
	{
	}

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	virtual void TickTask(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "CoreKit Actions|Actions")
	FGameplayTag ActionTag;

	UPROPERTY(VisibleAnywhere, Category = "CoreKit Actions|Actions")
	FGameplayTag ActivationInputTag;

	UPROPERTY(VisibleAnywhere, Category = "CoreKit Actions|Actions")
	float ElapsedTime = 0.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> OwnerActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCoreKitActionComponent> ActionComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCoreKitActionDefinition> Definition;

	/** Strong reference keeps a valid request payload alive through asynchronous execution. */
	UPROPERTY(Transient)
	TObjectPtr<UCoreKitActionContext> Context = nullptr;

	bool bFinishRequested = false;
	bool bCancelRequested = false;
	FGuid IASABlockHandle;
	bool bFinalized = false;
	ECoreKitActionEndReason PendingEndReason = ECoreKitActionEndReason::OwnerEndPlay;
	UPROPERTY(Transient)
	FGuid ExecutionId;
	UPROPERTY(Transient)
	FName ActionPhase = TEXT("Created");
};
