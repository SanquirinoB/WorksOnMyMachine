#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CoreKitActions/Actions/Types/CoreKitActionTypes.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "CoreKitActionDefinition.generated.h"

/** Data-driven activation and tag policy for one action. */
UCLASS(BlueprintType)
class COREKITACTIONS_API UCoreKitActionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** New voluntary gameplay requires global IASA; disable only for internal/forced progress. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IASA")
	bool bRequiresIASA = true;

	/** Close readiness on activation until an IASA notify or action completion releases it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IASA")
	bool bClosesIASAOnStart = false;

	/** Explicit policy for another request of the same action while it is alive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation")
	ECoreKitActionRepeatPolicy RepeatPolicy = ECoreKitActionRepeatPolicy::Reject;

	/** Opt in for new actions; historical end-tag changes otherwise retain their behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	bool bApplyEndTagsOnlyOnCompleted = false;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	/** Input intents that may select this action. The Actions module does not bind input itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation", meta = (Categories = "Input"))
	FGameplayTagContainer ActivationInputTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation")
	FGameplayTagContainer BlockedTags;

	/** Tags owned by the running action and automatically released when it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer GrantedTagsWhileActive;

	/** Legacy-only. Global tag removal is not supported; validation rejects authored values. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags", meta = (DeprecatedProperty, DeprecationMessage = "Use owner-scoped claims or a State transition."))
	FGameplayTagContainer TagsToClearOnStart;

	/** Loose state tags added by the component whenever the action ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer TagsToAddOnEnd;

	/** Legacy-only. Global tag removal is not supported; validation rejects authored values. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags", meta = (DeprecatedProperty, DeprecationMessage = "Use owner-scoped claims or a State transition."))
	FGameplayTagContainer TagsToClearOnEnd;

	/** Active actions whose action tags should be cancelled before this action starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cancellation")
	FGameplayTagContainer ActionsToCancel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cancellation")
	bool bCancelOtherActions = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Queue")
	bool bQueueWhenBlocked = false;

	/** Selection order when several actions consume the same input intent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activation")
	int32 SelectionPriority = 0;

	/** Replacement order inside the single-slot request queue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Queue")
	int32 QueuePriority = 0;

	/** Maximum time a blocked request may remain queued. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Queue",
		meta = (EditCondition = "bQueueWhenBlocked", ClampMin = "0.01", UIMin = "0.01"))
	float QueueWindowSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime")
	bool bTickAction = false;
};
