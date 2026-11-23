#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "CoreKitWaitForActionTag.generated.h"

class UCoreKitActionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoreKitWaitForTagCompleted);

UENUM(BlueprintType)
enum class ECoreKitWaitForActionTagResult : uint8
{
	Completed,
	Cancelled,
	TimedOut,
	InvalidSource,
	OwnerEnded,
	ComponentEnded
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FCoreKitWaitForTagFinished, ECoreKitWaitForActionTagResult, Result);

/** Blueprint async node that completes when an actor's Action Component gains a tag. */
UCLASS()
class COREKITACTIONS_API UCoreKitWaitForActionTag : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitWaitForTagCompleted OnCompleted;

	/** Always broadcasts exactly once, including invalid input, cancellation and teardown. */
	UPROPERTY(BlueprintAssignable, Category = "CoreKit Actions|Actions")
	FCoreKitWaitForTagFinished OnFinished;

	UFUNCTION(
		BlueprintCallable,
		meta = (BlueprintInternalUseOnly = "true"),
		Category = "CoreKit Actions|Actions"
	)
	static UCoreKitWaitForActionTag* WaitForActionTag(
		AActor* Actor,
		FGameplayTag TargetTag,
		float TimeoutSeconds = 0.0f
	);

	virtual void Activate() override;

	UFUNCTION(BlueprintCallable, Category = "CoreKit Actions|Actions")
	void Cancel();

	UFUNCTION(BlueprintPure, Category = "CoreKit Actions|Actions")
	ECoreKitWaitForActionTagResult GetResult() const { return Result; }

	virtual void BeginDestroy() override;

private:
	UFUNCTION()
	void HandleTagAdded(FGameplayTag AddedTag);

	UFUNCTION()
	void HandleOwnerEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void HandleActionComponentEnding();

	void HandleTimeout();

	void Finish(ECoreKitWaitForActionTagResult FinishResult);
	void Unbind();

	UPROPERTY()
	TObjectPtr<AActor> ObservedActor = nullptr;

	UPROPERTY()
	FGameplayTag TargetTag;

	UPROPERTY()
	float TimeoutSeconds = 0.0f;

	UPROPERTY()
	TWeakObjectPtr<UCoreKitActionComponent> ActionComponent;

	FTimerHandle TimeoutHandle;
	ECoreKitWaitForActionTagResult Result = ECoreKitWaitForActionTagResult::InvalidSource;

	bool bCompleted = false;
	bool bActivated = false;
};
