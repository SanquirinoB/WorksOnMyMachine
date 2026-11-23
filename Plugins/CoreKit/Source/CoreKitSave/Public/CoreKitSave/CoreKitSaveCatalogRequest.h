#pragma once

#include "CoreKitSave/CoreKitSaveRequest.h"
#include "CoreKitSaveCatalogRequest.generated.h"

USTRUCT(BlueprintType)
struct COREKITSAVE_API FCoreKitSaveCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	FCoreKitSaveSlotInfo Info;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	ECoreKitSaveResult Result = ECoreKitSaveResult::NotFound;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitSaveCatalogCompleted,
	ECoreKitSaveResult, Result, const TArray<FCoreKitSaveCatalogEntry>&, Entries);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCoreKitSaveCatalogCompletedNative,
	ECoreKitSaveResult, const TArray<FCoreKitSaveCatalogEntry>&);

UCLASS()
class COREKITSAVE_API UCoreKitSaveCatalogRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Save", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UCoreKitSaveCatalogRequest* ListSlots(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Save", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UCoreKitSaveCatalogRequest* GetSlotInfo(const UObject* WorldContextObject, const FString& SlotName);

	UPROPERTY(BlueprintAssignable)
	FCoreKitSaveCatalogCompleted Completed;

	FCoreKitSaveCatalogCompletedNative CompletedNative;

	virtual void Activate() override;

private:
	void Finish(ECoreKitSaveResult Result, const TArray<FCoreKitSaveCatalogEntry>& Entries);

	FString Slot;
	bool bList = true;
	bool bHasGameInstance = false;
	bool bActivated = false;
};
