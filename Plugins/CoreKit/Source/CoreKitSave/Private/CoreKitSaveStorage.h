#pragma once

#include "CoreKitSave/CoreKitSaveRequest.h"
#include "CoreKitSave/CoreKitSaveCatalogRequest.h"

namespace CoreKitSave
{
inline constexpr int32 MaxPayloadBytes = 32 * 1024 * 1024;

struct FDiskResult
{
	ECoreKitSaveResult Result = ECoreKitSaveResult::NotFound;
	FCoreKitSaveSlotInfo Info;
	TArray<uint8> Payload;
};

bool IsValidSlotName(const FString& SlotName);
FDiskResult ReadSlot(const FString& SlotName);
FDiskResult WriteSlot(const FString& SlotName, const TArray<uint8>& Payload);
FCoreKitSaveCatalogEntry ReadSlotInfo(const FString& SlotName);
TArray<FCoreKitSaveCatalogEntry> ListSlotInfo();
}
