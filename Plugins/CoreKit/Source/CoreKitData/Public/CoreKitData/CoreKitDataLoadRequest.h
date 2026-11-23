#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/SoftObjectPtr.h"
#include "CoreKitDataLoadRequest.generated.h"

class UCoreKitDataContainer;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitDataLoadCompleted,
	const TArray<UCoreKitDataContainer*>&, Loaded, const TArray<TSoftObjectPtr<UCoreKitDataContainer>>&, Missing);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCoreKitDataLoadCompletedNative,
	const TArray<UCoreKitDataContainer*>&, const TArray<TSoftObjectPtr<UCoreKitDataContainer>>&);

UCLASS()
class COREKITDATA_API UCoreKitDataLoadRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Data", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UCoreKitDataLoadRequest* LoadDataContainersAsync(const UObject* WorldContextObject,
		const TArray<TSoftObjectPtr<UCoreKitDataContainer>>& References);

	UPROPERTY(BlueprintAssignable)
	FCoreKitDataLoadCompleted Completed;

	FCoreKitDataLoadCompletedNative CompletedNative;

	virtual void Activate() override;

private:
	void Finish();

	TArray<TSoftObjectPtr<UCoreKitDataContainer>> Requested;
	TSharedPtr<FStreamableHandle> LoadHandle;
	bool bActivated = false;
	bool bHasGameInstance = false;
	bool bFinished = false;
};
