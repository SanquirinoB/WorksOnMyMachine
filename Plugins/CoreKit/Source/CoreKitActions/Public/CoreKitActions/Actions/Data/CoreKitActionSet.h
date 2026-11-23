#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "CoreKitActionSet.generated.h"

class UCoreKitAction;
class UCoreKitActionDefinition;

USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitActionEntry
{
	GENERATED_BODY()

	/** Instanced template. A fresh GameplayTask is created from its class per activation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Action")
	TObjectPtr<UCoreKitAction> ActionTemplate = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	TObjectPtr<UCoreKitActionDefinition> Definition = nullptr;
};

/** Reusable collection of action templates and their policies. */
UCLASS(BlueprintType)
class COREKITACTIONS_API UCoreKitActionSet : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer InitialTags;

	/** Action tags protected from cancellation by other actions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cancellation")
	FGameplayTagContainer NonCancelableActionTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TMap<FGameplayTag, FCoreKitActionEntry> Actions;

	/** Background actions started automatically at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TMap<FGameplayTag, FCoreKitActionEntry> PersistentActions;
};
