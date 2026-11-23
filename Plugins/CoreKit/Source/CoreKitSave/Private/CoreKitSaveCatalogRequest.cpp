#include "CoreKitSave/CoreKitSaveCatalogRequest.h"

#include "Async/Async.h"
#include "CoreKitSaveStorage.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UCoreKitSaveCatalogRequest* UCoreKitSaveCatalogRequest::ListSlots(const UObject* WorldContextObject)
{
	UCoreKitSaveCatalogRequest* Request = NewObject<UCoreKitSaveCatalogRequest>();
	if (UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		Request->bHasGameInstance = true;
		Request->RegisterWithGameInstance(Game);
	}
	return Request;
}

UCoreKitSaveCatalogRequest* UCoreKitSaveCatalogRequest::GetSlotInfo(
	const UObject* WorldContextObject, const FString& SlotName)
{
	UCoreKitSaveCatalogRequest* Request = ListSlots(WorldContextObject);
	Request->bList = false;
	Request->Slot = SlotName;
	return Request;
}

void UCoreKitSaveCatalogRequest::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;
	if (!bHasGameInstance || (!bList && !CoreKitSave::IsValidSlotName(Slot)))
	{
		Finish(ECoreKitSaveResult::InvalidRequest, {});
		return;
	}

	const TWeakObjectPtr<UCoreKitSaveCatalogRequest> WeakThis(this);
	Async(EAsyncExecution::ThreadPool, [WeakThis, bListSlots = bList, Name = Slot.ToLower()]
	{
		TArray<FCoreKitSaveCatalogEntry> Entries;
		ECoreKitSaveResult Result = ECoreKitSaveResult::Success;
		if (bListSlots)
		{
			Entries = CoreKitSave::ListSlotInfo();
		}
		else
		{
			FCoreKitSaveCatalogEntry Entry = CoreKitSave::ReadSlotInfo(Name);
			Result = Entry.Result;
			Entries.Add(MoveTemp(Entry));
		}

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Result, Entries = MoveTemp(Entries)]
		{
			if (UCoreKitSaveCatalogRequest* Request = WeakThis.Get())
			{
				Request->Finish(Result, Entries);
			}
		});
	});
}

void UCoreKitSaveCatalogRequest::Finish(ECoreKitSaveResult Result, const TArray<FCoreKitSaveCatalogEntry>& Entries)
{
	Completed.Broadcast(Result, Entries);
	CompletedNative.Broadcast(Result, Entries);
	SetReadyToDestroy();
}
