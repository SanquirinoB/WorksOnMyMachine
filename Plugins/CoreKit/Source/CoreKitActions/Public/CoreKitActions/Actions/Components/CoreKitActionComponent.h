#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTasksComponent.h"
#include "CoreKitActions/Actions/Types/CoreKitActionTypes.h"
#include "CoreKitActions/Actions/Types/CoreKitActiveTagLedger.h"
#include "CoreKitActions/Actions/Types/CoreKitActorComponentContext.h"
#include "CoreKitActions/Actions/Types/CoreKitInterruptibility.h"
#include "CoreKitActionComponent.generated.h"

class UCoreKitAction;
class UCoreKitActionContext;
class UCoreKitActionDefinition;
class UCoreKitActionSet;
class UCoreKitStateComponent;
class UActorComponent;
class UInterface;
struct FCoreKitActionEntry;
struct FBranchingPointNotifyPayload;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitActionTagEvent, FGameplayTag, Tag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitActionRequestEvent,
                                            const FCoreKitActionRequestReport&, Report);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FCoreKitActionStartedEvent,
	FGameplayTag, ActionTag,
	UCoreKitAction*, Action
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FCoreKitActionFinishedEvent,
	FGameplayTag, ActionTag,
	UCoreKitAction*, Action,
	ECoreKitActionEndReason, Reason
);

/** Broadcast once when this coordinator can no longer be observed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoreKitActionComponentEndingEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitIASAChangedEvent, bool, bIsIASA);

/**
 * Input-agnostic action coordinator and gameplay-tag state provider.
 * Input and domain modules request actions by tag; this component owns their lifecycle.
 */
UCLASS(ClassGroup = (AF), meta = (BlueprintSpawnableComponent))
class COREKITACTIONS_API UCoreKitActionComponent
	: public UGameplayTasksComponent
	  , public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	friend class UCoreKitAction;
	explicit UCoreKitActionComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit Actions|Actions")
	TObjectPtr<UCoreKitActionSet> ActionSet = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionTagEvent OnActiveTagAdded;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionTagEvent OnActiveTagRemoved;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionStartedEvent OnActionStarted;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionFinishedEvent OnActionFinished;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions|Debug")
	FCoreKitActionRequestEvent OnActionRequestEvaluated;

	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitActionComponentEndingEvent OnActionComponentEnding;

	/** Global actor readiness, including movement. Observers must recheck after external callbacks. */
	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|IASA")
	FCoreKitIASAChangedEvent OnIASAChanged;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|IASA")
	bool IsIASA() const { return bIASA && !bEndingPlay; }

	/** Closes global readiness until this exact handle is released or its source ends. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA")
	FGuid BlockIASA(UObject* Source, FName Reason);

	/** Releases only this handle. A retired action cannot release a newer grab's block. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA")
	bool ReleaseIASA(FGuid Handle);

	/** Observes one source-owned block, including whether its bound montage notify has released it. */
	bool HasIASABlock(FGuid Handle) const;

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA")
	void ReleaseIASABlocks(UObject* Source);

	/** Idempotent projection of an external state; montage notifies cannot release these blocks. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA")
	void SetIASAStateBlock(UObject* Source, FName Reason, bool bBlocked);

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|IASA|Debug")
	TArray<FName> GetIASABlockReasons() const;

#if WITH_EDITOR
	/** Returns a snapshot of the live IASA owners and reasons for editor diagnostics. */
	TArray<FString> DescribeIASABlocks() const;
#endif

	bool BindIASAToMontage(FGuid Handle, UAnimInstance* Animation, int32 MontageInstanceId);
	bool ReleaseIASAForMontage(UAnimInstance* Animation, int32 MontageInstanceId);

	/** Stops only playbacks that reached IASA, including unbound native-notify montages.
	 * Call after accepting a nonzero gameplay input. Closed global IASA is authoritative.
	 * Playback callbacks retain domain cleanup; already committed effects are not reversed.
	 * Returns the number interrupted. Replays/new playbacks created by callbacks are excluded.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|IASA", meta = (ClampMin = "0", Units = "s"))
	int32 InterruptIASAMontages(float BlendOutSeconds = 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit Actions|Actions|Debug")
	bool bLogActionRequests = false;

	/** Bounded per-component history; never persisted to SaveGame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit Actions|Actions|Debug",
		meta = (ClampMin = "0", ClampMax = "256"))
	int32 RequestHistoryLimit = 64;

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	FCoreKitActionRequestReport EvaluateAction(FGameplayTag ActionTag, FGameplayTag ActivationInputTag) const;

	/** Atomic synchronous command path. No GameplayTask/UObject allocation per sample.
	 * Readiness must be pure. Cleanup commands still validate ownership/configuration,
	 * but ignore start-only state restrictions so releasing input cannot get stuck.
	 * Continuous diagnostics are recorded only when the decision changes for that tag.
	 * bAllowNestedActionRequests releases only the command's dispatch guard, for adapters
	 * that synchronously open another domain session. Readiness remains guarded; nested
	 * requests still validate policy. Such adapters must guard their own reentrancy.
	 */
	FCoreKitActionRequestReport ExecuteActionCommand(FGameplayTag ActionTag, FGameplayTag InputTag,
	                                                        UActorComponent* Source,
	                                                        TFunctionRef<bool(FName&, FString&)> Readiness,
	                                                        TFunctionRef<bool(FName&, FString&)> Dispatch,
	                                                        bool bCleanup = false, bool bContinuous = false,
	                                                        bool bAllowNestedActionRequests = false);

	/** Read-only command policy query for prompts. No task, dispatch, tag writes or history entries. */
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|Actions|Diagnostics")
	FCoreKitActionRequestReport EvaluateCommandPolicy(FGameplayTag ActionTag,
	                                                         const UActorComponent* Source) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Debug")
	FCoreKitActionRequestReport GetLastRequestReport() const { return LastRequestReport; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Debug")
	TArray<FCoreKitActionRequestReport> GetRequestHistory() const { return RequestHistory; }

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Tags")
	void SetTagClaim(UObject* Source, FGameplayTag Tag, bool bActive);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Tags")
	void ReleaseTagClaims(UObject* Source);

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Debug")
	TArray<FString> GetTagSources(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|States")
	UCoreKitStateComponent* GetStateComponent() const { return StateComponent; }

	/** Native state-runtime handshake; never clears pre-existing legacy claims to take authority. */
	bool AttachStateComponent(UCoreKitStateComponent* States);
	bool CanReserveStateTags(const FGameplayTagContainer& Tags, FString& Reason) const;
	void RefreshAuthoritativeStates();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Requests a concrete action tag and reports why it was accepted or rejected. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	ECoreKitActionRequestResult RequestAction(FGameplayTag ActionTag, FGameplayTag ActivationInputTag);

	/** Full request API. The context is retained by a queued request and by its execution. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	FCoreKitActionRequestReport RequestActionWithContext(
		FGameplayTag ActionTag,
		FGameplayTag ActivationInputTag,
		UCoreKitActionContext* Context
	);

	/** Convenience wrapper: true only when the action starts immediately. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	bool TryActivateAction(FGameplayTag ActionTag, FGameplayTag ActivationInputTag);

	/** Selects the highest-priority action configured for an input intent tag. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	ECoreKitActionRequestResult HandleInputTag(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	bool CancelAction(FGameplayTag ActionTag, bool bForce = false);

	/** Cancels exactly one running execution, even if another execution reused its action tag. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	bool CancelExecution(FGuid ExecutionId, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void CancelAllActions(bool bIncludePersistentActions = false, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Queue")
	void ClearQueuedAction();

	/** Removes the queued request only when its identity matches. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Queue")
	bool CancelQueuedRequest(FGuid RequestId);

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Queue")
	FGameplayTag GetQueuedActionTag() const { return QueuedActionTag; }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Queue")
	FGuid GetQueuedRequestId() const { return QueuedRequestId; }

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Tags")
	void AddLooseTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions|Tags")
	void RemoveLooseTag(FGameplayTag Tag);

	/**
	 * Legacy compatibility helper. It releases only this component's loose claim; it never removes
	 * claims held by another owner. Use ReleaseTagClaims for an explicit owner.
	 */
	UFUNCTION(BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage = "Use ReleaseTagClaims with the owning object."), Category = "CoreKit Actions|Actions|Tags")
	void ClearActiveTag(FGameplayTag Tag);

	void AddActiveTagFromSource(FGameplayTag Tag, const void* Source);
	bool RemoveActiveTagFromSource(FGameplayTag Tag, const void* Source);
	void RemoveAllActiveTagsFromSource(const void* Source);
	bool HasActiveTagFromSource(FGameplayTag Tag, const void* Source) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Tags")
	bool HasActiveTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Tags")
	bool HasAnyActiveTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions|Tags")
	bool HasAllActiveTags(const FGameplayTagContainer& Tags) const;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	UCoreKitAction* GetActiveAction(FGameplayTag ActionTag) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	UCoreKitAction* GetCurrentAction() const { return CurrentAction.Get(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	TArray<UCoreKitAction*> GetActiveActions() const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	UCoreKitActionDefinition* GetActionDefinition(FGameplayTag ActionTag) const;

	/** Rebuild after adding, removing, or replacing a component on the owner actor. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actor Context")
	void RefreshActorContext();

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actor Context")
	int32 GetActorContextRevision() const { return ActorContext.GetRevision(); }

	/** A nameless query must match exactly one component. */
	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actor Context",
		meta = (DeterminesOutputType = "ComponentClass"))
	UActorComponent* GetCachedComponentByClass(TSubclassOf<UActorComponent> ComponentClass,
		ECoreKitActorComponentLookupResult& Result, FName ComponentName = NAME_None) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actor Context")
	UActorComponent* GetCachedComponentByInterface(TSubclassOf<UInterface> InterfaceClass,
		ECoreKitActorComponentLookupResult& Result, FName ComponentName = NAME_None) const;

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	FGameplayTagContainer GetActiveTags() const { return ActiveTags; }

	void HandleActionFinished(UCoreKitAction* Action, ECoreKitActionEndReason Reason);

protected:
	UCoreKitActionSet* ResolveActionSet() const;

private:
	UFUNCTION()
	void HandleIASAMontageNotify(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
	void UnbindIASAAnimations();
	void ObserveIASAAnimations();
	bool RememberInterruptibleMontage(UAnimInstance* Animation, int32 MontageInstanceId);
	TSet<TWeakObjectPtr<UAnimInstance>> IASAAnimations;
	TArray<FCoreKitIASAMontage> InterruptibleMontages;
	bool bInterruptingIASAMontages = false;
	void RefreshIASA();
	TMap<FGuid, FCoreKitIASABlock> IASABlocks;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "CoreKit Actions|IASA",
		meta = (AllowPrivateAccess = true))
	bool bIASA = true;

	void EnsureActorContext() const;

	const FCoreKitActionEntry* FindActionEntry(FGameplayTag ActionTag, bool& bOutPersistent) const;
	FCoreKitActionRequestReport RequestActionInternal(
		FGameplayTag ActionTag,
		FGameplayTag ActivationInputTag,
		bool bAllowQueue,
		bool bPersistent,
		UCoreKitActionContext* Context,
		FGuid RequestId
	);
	ECoreKitActionRequestResult StartAction(
		FGameplayTag ActionTag,
		FGameplayTag ActivationInputTag,
		const FCoreKitActionEntry& Entry,
		bool bPersistent,
		UCoreKitActionContext* Context,
		FCoreKitActionRequestReport& Report
	);
	bool QueueAction(FGameplayTag ActionTag, FGameplayTag ActivationInputTag, UCoreKitActionContext* Context,
		FGuid RequestId, int32 Priority, float WindowSeconds);
	bool ExpireQueuedActionIfNeeded();
	void ScheduleQueuedExpiry();
	void HandleQueuedExpiry();
	void RecordQueueTerminal(FGameplayTag ActionTag, FGameplayTag InputTag, FGuid RequestId,
		ECoreKitActionRequestResult Result, FName FailureCode, const FString& Detail);
	void TryActivateQueuedAction();
	static bool IsRetryableQueueResult(ECoreKitActionRequestResult Result);
	bool CanCancelActionTag(FGameplayTag ActionTag, bool bForce) const;
	void ApplyEndTagChanges(const UCoreKitActionDefinition& Definition);
	void BroadcastTagChanges(const FGameplayTagContainer& PreviousTags);
	void RefreshActiveTags();
	FCoreKitActionRequestReport EvaluatePolicy(FGameplayTag ActionTag, FGameplayTag InputTag,
	                                                  bool bInternalCommitCheck = false, bool bCleanup = false) const;
	void NormalizeReport(FCoreKitActionRequestReport& Report) const;
	void RecordRequest(FCoreKitActionRequestReport& Report);
	void PruneObjectClaims();
	void HandleStatesSettled();
	bool IsReservedStateWrite(FGameplayTag Tag) const;
	bool IsActionStillRegistered(const UCoreKitAction* Action, FGameplayTag ActionTag, bool bPersistent) const;
	void NotifyComponentEnding();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "CoreKit Actions|Actions|Tags",
		meta = (AllowPrivateAccess = true))
	FGameplayTagContainer ActiveTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "CoreKit Actions|Actions|States",
		meta = (AllowPrivateAccess = true))
	TObjectPtr<UCoreKitStateComponent> StateComponent = nullptr;
	bool bStateRuntimeRequired = false;

	UPROPERTY(Transient)
	mutable FCoreKitActorComponentContext ActorContext;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCoreKitAction>> ActiveActions;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCoreKitAction>> PersistentActions;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitAction> CurrentAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitAction> LastAction = nullptr;

	UPROPERTY(Transient)
	FGameplayTag QueuedActionTag;

	UPROPERTY(Transient)
	FGameplayTag QueuedInputTag;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitActionContext> QueuedContext = nullptr;

	UPROPERTY(Transient)
	FGuid QueuedRequestId;

	int32 QueuedPriority = TNumericLimits<int32>::Lowest();
	double QueuedExpiresAtSeconds = 0.0;
	FTimerHandle QueuedExpiryTimer;
	FGuid ProcessingQueuedRequestId;
	bool bProcessingQueuedRequestCancelled = false;
	int32 MutationDepth = 0;
	bool bEvaluatingActionPolicy = false;
	bool bInitialized = false;
	bool bProcessingQueue = false;
	FCoreKitActiveTagLedger ActiveTagLedger;
	UPROPERTY(Transient)
	FCoreKitActionRequestReport LastRequestReport;
	UPROPERTY(Transient)
	TArray<FCoreKitActionRequestReport> RequestHistory;
	TMap<FGameplayTag, FCoreKitActionRequestReport> ContinuousCommandReports;
	TMap<const void*, TWeakObjectPtr<UObject>> ObjectClaimSources;
	bool bEndingPlay = false;
	bool bComponentEndingNotified = false;
};
