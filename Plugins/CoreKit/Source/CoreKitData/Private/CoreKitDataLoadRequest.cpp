#include "CoreKitData/CoreKitDataLoadRequest.h"

#include "CoreKitData/CoreKitDataContainer.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"

UCoreKitDataLoadRequest* UCoreKitDataLoadRequest::LoadDataContainersAsync(const UObject* WorldContextObject,
	const TArray<TSoftObjectPtr<UCoreKitDataContainer>>& References)
{
	auto* Request = NewObject<UCoreKitDataLoadRequest>();
	Request->Requested = References;
	if (UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		Request->bHasGameInstance = true;
		Request->RegisterWithGameInstance(Game);
	}
	return Request;
}

void UCoreKitDataLoadRequest::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	TArray<FSoftObjectPath> Paths;
	TSet<FSoftObjectPath> Seen;
	for (const auto& Reference : Requested)
	{
		const FSoftObjectPath& Path = Reference.ToSoftObjectPath();
		if (!Path.IsNull() && !Seen.Contains(Path))
		{
			Seen.Add(Path);
			Paths.Add(Path);
		}
	}

	if (bHasGameInstance && !Paths.IsEmpty())
	{
		LoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(Paths),
			FStreamableDelegate::CreateUObject(this, &ThisClass::Finish));
	}
	if (!LoadHandle)
	{
		Finish();
	}
}

void UCoreKitDataLoadRequest::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	TArray<UCoreKitDataContainer*> Loaded;
	TArray<TSoftObjectPtr<UCoreKitDataContainer>> Missing;
	for (const auto& Reference : Requested)
	{
		if (UCoreKitDataContainer* Container = Reference.Get())
		{
			Loaded.Add(Container);
		}
		else
		{
			Missing.Add(Reference);
		}
	}

	Completed.Broadcast(Loaded, Missing);
	CompletedNative.Broadcast(Loaded, Missing);
	LoadHandle.Reset();
	SetReadyToDestroy();
}
