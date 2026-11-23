#include "CoreKitSave/CoreKitSaveRequest.h"

#include "Async/Async.h"
#include "CoreKitSaveStorage.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Kismet/GameplayStatics.h"

UCoreKitSaveRequest* UCoreKitSaveRequest::LoadCoreKitSlot(const UObject* WorldContextObject, const FString& SlotName)
{
	UCoreKitSaveRequest* Request = NewObject<UCoreKitSaveRequest>();
	Request->Slot = SlotName;
	if (UGameInstance* Game = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		Request->bHasGameInstance = true;
		Request->RegisterWithGameInstance(Game);
	}
	return Request;
}

UCoreKitSaveRequest* UCoreKitSaveRequest::SaveCoreKitSlot(
	const UObject* WorldContextObject, const FString& SlotName, USaveGame* SaveGame)
{
	UCoreKitSaveRequest* Request = LoadCoreKitSlot(WorldContextObject, SlotName);
	Request->bWrite = true;
	Request->SavePayload = SaveGame;
	return Request;
}

void UCoreKitSaveRequest::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	if (!bHasGameInstance || !CoreKitSave::IsValidSlotName(Slot) || (bWrite && !IsValid(SavePayload)))
	{
		Finish(ECoreKitSaveResult::InvalidRequest, nullptr, {});
		return;
	}

	TArray<uint8> Payload;
	if (bWrite && (!UGameplayStatics::SaveGameToMemory(SavePayload, Payload)
		|| Payload.IsEmpty() || Payload.Num() > CoreKitSave::MaxPayloadBytes))
	{
		Finish(ECoreKitSaveResult::InvalidRequest, nullptr, {});
		return;
	}

	SavePayload = nullptr;
	const TWeakObjectPtr<UCoreKitSaveRequest> WeakThis(this);
	Async(EAsyncExecution::ThreadPool,
		[WeakThis, SlotName = Slot.ToLower(), Payload = MoveTemp(Payload), bSaving = bWrite]()
		{
			CoreKitSave::FDiskResult Data = bSaving ? CoreKitSave::WriteSlot(SlotName, Payload) : CoreKitSave::ReadSlot(SlotName);
			AsyncTask(ENamedThreads::GameThread, [WeakThis, Data = MoveTemp(Data), bSaving]() mutable
			{
				if (UCoreKitSaveRequest* Request = WeakThis.Get())
				{
					USaveGame* Loaded = nullptr;
					if (!bSaving && Data.Result == ECoreKitSaveResult::Success)
					{
						Loaded = UGameplayStatics::LoadGameFromMemory(Data.Payload);
						if (!Loaded)
						{
							Data.Result = ECoreKitSaveResult::Corrupt;
						}
					}
					Request->Finish(Data.Result, Loaded, Data.Info);
				}
			});
		});
}

void UCoreKitSaveRequest::Finish(ECoreKitSaveResult Result, USaveGame* LoadedGame, const FCoreKitSaveSlotInfo& SlotInfo)
{
	Completed.Broadcast(Result, LoadedGame, SlotInfo);
	CompletedNative.Broadcast(Result, LoadedGame, SlotInfo);
	SetReadyToDestroy();
}
