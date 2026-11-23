#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CoreKitStateTypes.generated.h"

UENUM(BlueprintType)
enum class ECoreKitStateResult : uint8
{
	Success, InvalidDefinition, DuplicateDomain, OverlappingDomain, LegacyClaimConflict,
	InvalidAuthority, UnknownDomain, StaleAuthority, StaleRevision, StaleExecution,
	UnexpectedState, InvalidTransition, MissingRequirements, Blocked, ReadOnlyProvider,
	ProviderUnavailable, TransitionInProgress, OwnerEndingPlay
};

/** Runtime capability. A handle from an earlier registration never authorizes a new one. */
USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateAuthority
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="State")
	FName Domain;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGuid Generation;
	bool IsValid() const { return !Domain.IsNone() && Generation.IsValid(); }
};

USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateTransitionRule
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(Categories="State"))
	FGameplayTag From;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(Categories="Event"))
	FGameplayTag Event;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition", meta=(Categories="State"))
	FGameplayTag To;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition")
	FGameplayTagContainer RequiredTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Transition")
	FGameplayTagContainer BlockedTags;
};

/** Definition is copied at registration: editing an asset cannot rewrite a live state machine. */
USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateDomainDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="State")
	FName Domain;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="State", meta=(Categories="State"))
	FGameplayTagContainer States;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="State", meta=(Categories="State"))
	FGameplayTag InitialState;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="State")
	TArray<FCoreKitStateTransitionRule> Transitions;
	bool Validate(FString& Reason, bool bProvider = false) const;
};

USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateDomainSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="State")
	FName Domain;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTag State;
	UPROPERTY(BlueprintReadOnly, Category="State")
	int64 Revision = 0;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGuid ExecutionId;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FString AuthorityPath;
	UPROPERTY(BlueprintReadOnly, Category="State")
	double EnteredAtSeconds = 0.0;
	UPROPERTY(BlueprintReadOnly, Category="State")
	bool bProvider = false;
	UPROPERTY(BlueprintReadOnly, Category="State")
	bool bAuthorityAvailable = false;
};

USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateSnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="State")
	int64 Revision = 0;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTagContainer Tags;
	UPROPERTY(BlueprintReadOnly, Category="State")
	TArray<FCoreKitStateDomainSnapshot> Domains;
};

/** Capture expectations when scheduling work, not when a delayed callback finally fires. */
USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateTransitionRequest
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	FCoreKitStateAuthority Authority;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	TObjectPtr<UObject> Writer = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	FGameplayTag ExpectedState;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	int64 ExpectedRevision = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	FGuid ExpectedExecutionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	FGameplayTag Event;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	FGameplayTag To;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="State")
	FGuid ExecutionId;
};

USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitStateReport
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="State")
	ECoreKitStateResult Result = ECoreKitStateResult::InvalidDefinition;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FName Domain;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTag From;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTag Event;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTag To;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGuid ExecutionId;
	UPROPERTY(BlueprintReadOnly, Category="State")
	int64 SnapshotRevision = 0;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FString AuthorityPath;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FString Detail;
	UPROPERTY(BlueprintReadOnly, Category="State")
	double TimeSeconds = 0.0;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTagContainer MissingTags;
	UPROPERTY(BlueprintReadOnly, Category="State")
	FGameplayTagContainer BlockingTags;
};
