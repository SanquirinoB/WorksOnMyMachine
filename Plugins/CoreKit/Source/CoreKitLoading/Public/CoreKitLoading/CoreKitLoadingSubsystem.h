#pragma once

#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "CoreKitLoadingSubsystem.generated.h"

class SWidget;
class UCoreKitOpenLevelRequest;
class UGameViewportClient;
class UUserWidget;
struct FWorldContext;

USTRUCT(BlueprintType)
struct COREKITLOADING_API FCoreKitLoadingRequestInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Loading")
	FGuid Id;
	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Loading")
	FString Owner;
	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Loading")
	FText Reason;
	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Loading")
	double ElapsedSeconds = 0.0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitLoadingVisibilityChanged, bool, bVisible);

UCLASS()
class COREKITLOADING_API UCoreKitLoadingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Loading")
	void ConfigureLoadingScreen(TSubclassOf<UUserWidget> WidgetClass, bool bTrackMapTravel = true);

	/** Returns a request token. End that token on success, failure, or cancellation. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Loading")
	FGuid BeginLoading();

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Loading")
	FGuid BeginLoadingRequest(UObject* Owner, FText Reason);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Loading")
	bool EndLoading(FGuid Request);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Loading")
	bool IsLoading() const { return bTravelInProgress || !ActiveRequests.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Loading")
	bool IsLoadingScreenVisible() const { return Overlay.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Loading")
	TArray<FCoreKitLoadingRequestInfo> GetActiveLoadingRequests() const;

	UFUNCTION(BlueprintPure, Category = "CoreKit|Loading")
	FGuid GetActiveTravelId() const;

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Loading")
	bool ResolveTravelCondition(FGuid TravelId, FName Condition, bool bSucceeded);

	UPROPERTY(BlueprintAssignable, Category = "CoreKit|Loading")
	FCoreKitLoadingVisibilityChanged OnLoadingScreenVisibilityChanged;

private:
	friend class UCoreKitOpenLevelRequest;

	void OnPreLoadMap(const FWorldContext& Context, const FString& MapName);
	void OnPostLoadMap(UWorld* World);
	void OnTravelFailure(UWorld* World, ETravelFailure::Type Failure, const FString& Message);
	void RefreshPresentation();
	void UpdatePresentation();
	void HidePresentation();
	bool HasOverlayPainted() const;

	struct FPreviousFocus
	{
		uint32 UserIndex;
		TWeakPtr<SWidget> Widget;
	};

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> ScreenClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Screen;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitOpenLevelRequest> ActiveTravel;

	struct FLoadingRequest
	{
		FString Owner;
		FText Reason;
		double StartedAt = 0.0;
	};

	TMap<FGuid, FLoadingRequest> ActiveRequests;
	TWeakObjectPtr<UGameViewportClient> AttachedViewport;
	TSharedPtr<SWidget> Overlay;
	TArray<FPreviousFocus> PreviousFocus;
	bool bTrackTravel = false;
	bool bTravelInProgress = false;
	bool bPreviousIgnoreInput = false;
	bool bRefreshing = false;
	bool bPublishedVisible = false;
	bool bShuttingDown = false;
};
