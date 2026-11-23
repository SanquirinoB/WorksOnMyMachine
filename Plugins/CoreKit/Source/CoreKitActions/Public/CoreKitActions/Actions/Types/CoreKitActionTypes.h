#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoreKitActionTypes.generated.h"

UENUM(BlueprintType)
enum class ECoreKitActionEndReason : uint8
{
	Completed,
	Cancelled,
	ExternalCancelled,
	Replaced,
	OwnerEndPlay,
	Failed
};

UENUM(BlueprintType)
enum class ECoreKitActionRequestResult : uint8
{
	Activated,
	Queued,
	ActionNotFound,
	MissingRequirements,
	BlockedByTags,
	RejectedByAction,
	BlockedByNonCancelableAction,
	InvalidConfiguration,
	AlreadyActive,
	TransitionInProgress,
	QueueRejected,
	RequestExpired,
	InvalidContext
};

/** High-level lifecycle meaning; Result retains the detailed decision/rejection code. */
UENUM(BlueprintType)
enum class ECoreKitActionRequestStatus : uint8
{
	Allowed,
	Started,
	Executed,
	Queued,
	Rejected
};

/** Replace preserves the original behavior. New modal actions should use Reject. */
UENUM(BlueprintType)
enum class ECoreKitActionRepeatPolicy : uint8
{
	Replace,
	Reject
};

/** One request's snapshot. Queued is not the same as started or completed. */
USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitActionRequestReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGameplayTag ActionTag;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGameplayTag InputTag;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	ECoreKitActionRequestResult Result = ECoreKitActionRequestResult::InvalidConfiguration;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	ECoreKitActionRequestStatus Status = ECoreKitActionRequestStatus::Rejected;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGameplayTagContainer MissingTags;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGameplayTagContainer BlockingTags;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGameplayTagContainer ConflictingActions;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	TArray<FString> BlockingSources;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FName FailureCode;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FString Detail;
	/** Stable identity shared by the initial request and every queued retry. */
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGuid RequestId;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	FGuid ExecutionId;
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	double TimeSeconds = 0.0;
	/** Revision of the authoritative state snapshot used for this decision; zero for legacy. */
	UPROPERTY(BlueprintReadOnly, Category = "Actions")
	int64 StateRevision = 0;
};
