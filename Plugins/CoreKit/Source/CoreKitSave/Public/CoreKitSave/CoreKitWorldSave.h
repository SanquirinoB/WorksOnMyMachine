#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoreKitWorldSave.generated.h"

USTRUCT(BlueprintType)
struct COREKITSAVE_API FCoreKitParticipantState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Save")
	int32 Version = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Save")
	FInstancedStruct Payload;
};

USTRUCT()
struct FCoreKitParticipantRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FCoreKitParticipantState State;
};

UCLASS(Blueprintable)
class COREKITSAVE_API UCoreKitWorldSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CoreKit|Save")
	FName GetSavedWorldPackage() const { return WorldPackage; }

private:
	friend class UCoreKitWorldSaveSubsystem;

	UPROPERTY()
	FName WorldPackage;

	UPROPERTY()
	TArray<FCoreKitParticipantRecord> Participants;
};

UENUM(BlueprintType)
enum class ECoreKitWorldSaveResult : uint8
{
	Success,
	SuccessWithMissing,
	WaitingForParticipants,
	InvalidSnapshot,
	InvalidId,
	DuplicateId,
	ParticipantRejected,
	Busy
};

USTRUCT(BlueprintType)
struct COREKITSAVE_API FCoreKitWorldSaveReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	ECoreKitWorldSaveResult Result = ECoreKitWorldSaveResult::Success;

	UPROPERTY(BlueprintReadOnly, Category = "CoreKit|Save")
	TArray<FGuid> ParticipantIds;
};

/** Add a subclass to each actor that opts into world capture and restore. */
UCLASS(Abstract, Blueprintable, ClassGroup = (CoreKit), meta = (BlueprintSpawnableComponent))
class COREKITSAVE_API UCoreKitSaveParticipantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CoreKit|Save")
	FGuid GetPersistenceId() const { return PersistenceId; }

	UFUNCTION(CallInEditor, Category = "CoreKit|Save")
	void GeneratePersistenceId();

	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Save")
	bool CaptureSaveState(FCoreKitParticipantState& OutState);

	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Save")
	bool CanRestoreSaveState(const FCoreKitParticipantState& State);

	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Save")
	void ApplySaveState(const FCoreKitParticipantState& State);

protected:
	/** Assign once on a placed actor. Duplicated actors require a new ID. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "CoreKit|Save")
	FGuid PersistenceId;
};

UCLASS()
class COREKITSAVE_API UCoreKitWorldSaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Save")
	FCoreKitWorldSaveReport CaptureWorld(UCoreKitWorldSaveGame* Snapshot);

	/** Retry after the destination's actors have begun play. Waiting applies no state. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Save")
	FCoreKitWorldSaveReport RestoreWorld(UCoreKitWorldSaveGame* Snapshot, bool bAllowMissing = false);

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	FCoreKitWorldSaveReport CollectParticipants(TMap<FGuid, TWeakObjectPtr<UCoreKitSaveParticipantComponent>>& OutParticipants) const;
	FName GetWorldPackage() const;

	bool bProcessing = false;
};
