#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "Misc/DateTime.h"
#include "CoreKitSaveRequest.generated.h"

class USaveGame;

UENUM(BlueprintType)
enum class ECoreKitSaveResult : uint8
{
	Success,
	InvalidRequest,
	NotFound,
	Corrupt,
	UnsupportedVersion,
	IOError
};

USTRUCT(BlueprintType)
struct COREKITSAVE_API FCoreKitSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	FDateTime WrittenAtUtc;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	int32 FormatVersion = 0;

	/** True after the stored record has passed the full checksum check. */
	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	bool bVerified = false;

	/** A verified bank was loaded while the other bank was damaged or unreadable. */
	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	bool bRecoveredBackup = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCoreKitSaveCompleted,
	ECoreKitSaveResult, Result, USaveGame*, LoadedGame, const FCoreKitSaveSlotInfo&, SlotInfo);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCoreKitSaveCompletedNative,
	ECoreKitSaveResult, USaveGame*, const FCoreKitSaveSlotInfo&);

UCLASS()
class COREKITSAVE_API UCoreKitSaveRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Save", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UCoreKitSaveRequest* SaveCoreKitSlot(const UObject* WorldContextObject, const FString& SlotName, USaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Save", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UCoreKitSaveRequest* LoadCoreKitSlot(const UObject* WorldContextObject, const FString& SlotName);

	UPROPERTY(BlueprintAssignable)
	FCoreKitSaveCompleted Completed;

	FCoreKitSaveCompletedNative CompletedNative;

	virtual void Activate() override;

private:
	void Finish(ECoreKitSaveResult Result, USaveGame* LoadedGame, const FCoreKitSaveSlotInfo& SlotInfo);

	UPROPERTY(Transient)
	TObjectPtr<USaveGame> SavePayload;

	FString Slot;
	bool bWrite = false;
	bool bHasGameInstance = false;
	bool bActivated = false;
};
