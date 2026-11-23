#include "CoreKitLoading/CoreKitOpenLevelRequest.h"

#include "CoreKitLoading/CoreKitLoadingSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

UCoreKitOpenLevelRequest* UCoreKitOpenLevelRequest::OpenLevelWithLoading(const UObject* WorldContextObject,
	TSoftObjectPtr<UWorld> Level, const TArray<FName>& ReadyConditions, float TimeoutSeconds)
{
	auto* Request = NewObject<UCoreKitOpenLevelRequest>();
	Request->Destination = Level;
	Request->Timeout = TimeoutSeconds;
	for (const FName Condition : ReadyConditions)
	{
		if (!Condition.IsNone() && Condition != TEXT("MapLoaded"))
		{
			Request->PendingConditions.Add(Condition);
		}
	}

	if (UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		Request->Subsystem = Game->GetSubsystem<UCoreKitLoadingSubsystem>();
		Request->RegisterWithGameInstance(Game);
	}
	return Request;
}

void UCoreKitOpenLevelRequest::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	const FString PackageName = Destination.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::IsValidLongPackageName(PackageName) || !FPackageName::DoesPackageExist(PackageName))
	{
		Finish(ECoreKitTravelResult::InvalidDestination);
		return;
	}

	UCoreKitLoadingSubsystem* Loading = Subsystem.Get();
	if (!Loading || Loading->bShuttingDown || !FMath::IsFinite(Timeout) || Timeout <= 0.0f)
	{
		Finish(ECoreKitTravelResult::Unavailable);
		return;
	}
	if (Loading->ActiveTravel)
	{
		Finish(ECoreKitTravelResult::Busy);
		return;
	}

	Loading->ActiveTravel = this;
	TravelId = Loading->BeginLoadingRequest(this, FText::FromString(PackageName));
	Viewport = Loading->GetGameInstance()->GetGameViewportClient();
	if (!Viewport.IsValid() || !Loading->IsLoadingScreenVisible())
	{
		Finish(ECoreKitTravelResult::Unavailable);
		return;
	}

	DrawHandle = Viewport->OnEndDraw().AddUObject(this, &ThisClass::DispatchTravel);
	TimeoutHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		TimeoutHandle.Reset();
		Finish(ECoreKitTravelResult::TimedOut);
		return false;
	}), Timeout);
}

void UCoreKitOpenLevelRequest::DispatchTravel()
{
	UCoreKitLoadingSubsystem* Loading = Subsystem.Get();
	if (!Loading || !Loading->IsLoadingScreenVisible())
	{
		Finish(ECoreKitTravelResult::Unavailable);
		return;
	}
	// The viewport's first draw can finish before Slate paints the overlay.
	if (!Loading->HasOverlayPainted())
	{
		return;
	}

	if (UGameViewportClient* Client = Viewport.Get())
	{
		Client->OnEndDraw().Remove(DrawHandle);
	}
	DrawHandle.Reset();

	bDispatched = true;
	UGameplayStatics::OpenLevelBySoftObjectPtr(Loading->GetGameInstance(), Destination);
}

void UCoreKitOpenLevelRequest::HandleMapLoaded(UWorld* World)
{
	if (!bDispatched || bMapLoaded || bFinished)
	{
		return;
	}

	const FString LoadedPackage = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	if (LoadedPackage != Destination.ToSoftObjectPath().GetLongPackageName())
	{
		Finish(ECoreKitTravelResult::TravelFailed);
		return;
	}

	bMapLoaded = true;
	bNotifyingMapLoaded = true;
	MapLoaded.Broadcast(ECoreKitTravelResult::MapLoaded, TravelId);
	MapLoadedNative.Broadcast(ECoreKitTravelResult::MapLoaded, TravelId);
	bNotifyingMapLoaded = false;
	TryComplete();
}

bool UCoreKitOpenLevelRequest::ResolveCondition(FName Condition, bool bSucceeded)
{
	if (!bDispatched || bFinished || PendingConditions.Remove(Condition) == 0)
	{
		return false;
	}

	bConditionFailed |= !bSucceeded;
	TryComplete();
	return true;
}

void UCoreKitOpenLevelRequest::TryComplete()
{
	if (!bMapLoaded || bNotifyingMapLoaded || bFinished)
	{
		return;
	}
	if (bConditionFailed)
	{
		Finish(ECoreKitTravelResult::ReadinessFailed);
	}
	else if (PendingConditions.IsEmpty())
	{
		Finish(ECoreKitTravelResult::Success);
	}
}

void UCoreKitOpenLevelRequest::Finish(ECoreKitTravelResult Result)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (DrawHandle.IsValid())
	{
		if (UGameViewportClient* Client = Viewport.Get())
		{
			Client->OnEndDraw().Remove(DrawHandle);
		}
	}
	if (TimeoutHandle.IsValid())
	{
		FTSTicker::RemoveTicker(TimeoutHandle);
	}

	if (UCoreKitLoadingSubsystem* Loading = Subsystem.Get(); Loading && Loading->ActiveTravel == this)
	{
		Loading->ActiveTravel = nullptr;
		if (bDispatched && !bMapLoaded)
		{
			Loading->bTravelInProgress = false;
		}
		Loading->EndLoading(TravelId);
	}

	Completed.Broadcast(Result, TravelId);
	CompletedNative.Broadcast(Result, TravelId);
	SetReadyToDestroy();
}
