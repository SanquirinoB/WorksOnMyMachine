#pragma once

#include "Containers/Ticker.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/SoftObjectPtr.h"
#include "CoreKitOpenLevelRequest.generated.h"

class UCoreKitLoadingSubsystem;
class UGameViewportClient;
class UWorld;

UENUM(BlueprintType)
enum class ECoreKitTravelResult : uint8
{
	MapLoaded,
	Success,
	InvalidDestination,
	Unavailable,
	Busy,
	TravelFailed,
	ReadinessFailed,
	TimedOut,
	Cancelled
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitTravelEvent,
	ECoreKitTravelResult, Result, FGuid, TravelId);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCoreKitTravelEventNative, ECoreKitTravelResult, FGuid);

UCLASS()
class COREKITLOADING_API UCoreKitOpenLevelRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Loading", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UCoreKitOpenLevelRequest* OpenLevelWithLoading(const UObject* WorldContextObject,
		TSoftObjectPtr<UWorld> Level, const TArray<FName>& ReadyConditions, float TimeoutSeconds = 30.0f);

	UPROPERTY(BlueprintAssignable)
	FCoreKitTravelEvent MapLoaded;

	UPROPERTY(BlueprintAssignable)
	FCoreKitTravelEvent Completed;

	FCoreKitTravelEventNative MapLoadedNative;
	FCoreKitTravelEventNative CompletedNative;

	virtual void Activate() override;

private:
	friend class UCoreKitLoadingSubsystem;

	void DispatchTravel();
	void HandleMapLoaded(UWorld* World);
	bool ResolveCondition(FName Condition, bool bSucceeded);
	void TryComplete();
	void Finish(ECoreKitTravelResult Result);

	TWeakObjectPtr<UCoreKitLoadingSubsystem> Subsystem;
	TWeakObjectPtr<UGameViewportClient> Viewport;
	TSoftObjectPtr<UWorld> Destination;
	TSet<FName> PendingConditions;
	FDelegateHandle DrawHandle;
	FTSTicker::FDelegateHandle TimeoutHandle;
	FGuid TravelId;
	float Timeout = 30.0f;
	bool bActivated = false;
	bool bDispatched = false;
	bool bMapLoaded = false;
	bool bNotifyingMapLoaded = false;
	bool bConditionFailed = false;
	bool bFinished = false;
};
