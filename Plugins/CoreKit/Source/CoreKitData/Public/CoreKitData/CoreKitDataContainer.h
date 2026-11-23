#pragma once

#include "CoreKitData/CoreKitDataFragment.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "CoreKitDataContainer.generated.h"

USTRUCT(BlueprintType)
struct COREKITDATA_API FCoreKitFragmentRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "CoreKit|Data")
	TSubclassOf<UCoreKitDataFragment> FragmentClass;
	UPROPERTY(EditAnywhere, Category = "CoreKit|Data", meta = (ClampMin = "0"))
	int32 Minimum = 1;
	/** Use -1 for an unlimited number of matching fragments. */
	UPROPERTY(EditAnywhere, Category = "CoreKit|Data", meta = (ClampMin = "-1"))
	int32 Maximum = 1;
};

/** Authored configuration composed of inline fragment objects. */
UCLASS(BlueprintType, Blueprintable)
class COREKITDATA_API UCoreKitDataContainer : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Returns the first fragment compatible with FragmentClass in authored order, or null. */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Data", meta = (DeterminesOutputType = "FragmentClass"))
	UCoreKitDataFragment* FindDataFragment(TSubclassOf<UCoreKitDataFragment> FragmentClass) const;

	/** Returns every fragment compatible with FragmentClass in authored order. */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Data", meta = (DeterminesOutputType = "FragmentClass"))
	TArray<UCoreKitDataFragment*> FindDataFragments(TSubclassOf<UCoreKitDataFragment> FragmentClass) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	/** The order controls which fragment is returned by FindDataFragment. Treat these objects as read-only at runtime. */
	UPROPERTY(EditAnywhere, Instanced, Category = "CoreKit|Data")
	TArray<TObjectPtr<UCoreKitDataFragment>> Fragments;

	UPROPERTY(EditDefaultsOnly, Category = "CoreKit|Data")
	TArray<FCoreKitFragmentRule> FragmentRules;
};
