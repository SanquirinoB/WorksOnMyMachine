#include "CoreKitSave/CoreKitWorldSave.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/Package.h"

void UCoreKitSaveParticipantComponent::GeneratePersistenceId()
{
#if WITH_EDITOR
	const UWorld* World = GetWorld();
	if (!IsTemplate() && World && World->WorldType == EWorldType::Editor)
	{
		Modify();
		PersistenceId = FGuid::NewGuid();
		MarkPackageDirty();
	}
#endif
}

bool UCoreKitSaveParticipantComponent::CaptureSaveState_Implementation(FCoreKitParticipantState&)
{
	return false;
}

bool UCoreKitSaveParticipantComponent::CanRestoreSaveState_Implementation(const FCoreKitParticipantState&)
{
	return false;
}

void UCoreKitSaveParticipantComponent::ApplySaveState_Implementation(const FCoreKitParticipantState&)
{
}

bool UCoreKitWorldSaveSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

FName UCoreKitWorldSaveSubsystem::GetWorldPackage() const
{
	return FName(UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName()));
}

FCoreKitWorldSaveReport UCoreKitWorldSaveSubsystem::CollectParticipants(
	TMap<FGuid, TWeakObjectPtr<UCoreKitSaveParticipantComponent>>& OutParticipants) const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UCoreKitSaveParticipantComponent*> Components(*It);
		for (UCoreKitSaveParticipantComponent* Component : Components)
		{
			if (!IsValid(Component) || !Component->HasBegunPlay())
			{
				continue;
			}

			const FGuid Id = Component->GetPersistenceId();
			if (!Id.IsValid())
			{
				return { ECoreKitWorldSaveResult::InvalidId, { Id } };
			}
			if (OutParticipants.Contains(Id))
			{
				return { ECoreKitWorldSaveResult::DuplicateId, { Id } };
			}
			OutParticipants.Add(Id, Component);
		}
	}
	return {};
}

FCoreKitWorldSaveReport UCoreKitWorldSaveSubsystem::CaptureWorld(UCoreKitWorldSaveGame* Snapshot)
{
	if (bProcessing)
	{
		return { ECoreKitWorldSaveResult::Busy };
	}
	if (!IsValid(Snapshot))
	{
		return { ECoreKitWorldSaveResult::InvalidSnapshot };
	}

	TGuardValue<bool> Guard(bProcessing, true);
	TMap<FGuid, TWeakObjectPtr<UCoreKitSaveParticipantComponent>> Participants;
	const FCoreKitWorldSaveReport Report = CollectParticipants(Participants);
	if (Report.Result != ECoreKitWorldSaveResult::Success)
	{
		return Report;
	}

	TArray<FGuid> Ids;
	Participants.GenerateKeyArray(Ids);
	Ids.Sort();

	TArray<FCoreKitParticipantRecord> Records;
	Records.Reserve(Ids.Num());
	for (const FGuid& Id : Ids)
	{
		UCoreKitSaveParticipantComponent* Component = Participants.FindChecked(Id).Get();
		FCoreKitParticipantRecord Record;
		Record.Id = Id;
		if (!Component || !Component->CaptureSaveState(Record.State) || Record.State.Version <= 0 || !Record.State.Payload.IsValid())
		{
			return { ECoreKitWorldSaveResult::ParticipantRejected, { Id } };
		}
		Records.Add(MoveTemp(Record));
	}

	Snapshot->WorldPackage = GetWorldPackage();
	Snapshot->Participants = MoveTemp(Records);
	return {};
}

FCoreKitWorldSaveReport UCoreKitWorldSaveSubsystem::RestoreWorld(UCoreKitWorldSaveGame* Snapshot, bool bAllowMissing)
{
	if (bProcessing)
	{
		return { ECoreKitWorldSaveResult::Busy };
	}
	if (!IsValid(Snapshot) || Snapshot->WorldPackage != GetWorldPackage())
	{
		return { ECoreKitWorldSaveResult::InvalidSnapshot };
	}

	TGuardValue<bool> Guard(bProcessing, true);
	TMap<FGuid, TWeakObjectPtr<UCoreKitSaveParticipantComponent>> Participants;
	FCoreKitWorldSaveReport Report = CollectParticipants(Participants);
	if (Report.Result != ECoreKitWorldSaveResult::Success)
	{
		return Report;
	}

	TSet<FGuid> Seen;
	TArray<const FCoreKitParticipantRecord*> Ready;
	for (const FCoreKitParticipantRecord& Record : Snapshot->Participants)
	{
		if (!Record.Id.IsValid() || Record.State.Version <= 0 || !Record.State.Payload.IsValid())
		{
			return { ECoreKitWorldSaveResult::InvalidSnapshot, { Record.Id } };
		}
		if (Seen.Contains(Record.Id))
		{
			return { ECoreKitWorldSaveResult::DuplicateId, { Record.Id } };
		}
		Seen.Add(Record.Id);

		const TWeakObjectPtr<UCoreKitSaveParticipantComponent>* Entry = Participants.Find(Record.Id);
		if (!Entry || !Entry->IsValid())
		{
			Report.ParticipantIds.Add(Record.Id);
			continue;
		}
		Ready.Add(&Record);
	}

	if (!bAllowMissing && !Report.ParticipantIds.IsEmpty())
	{
		Report.Result = ECoreKitWorldSaveResult::WaitingForParticipants;
		return Report;
	}

	for (const FCoreKitParticipantRecord* Record : Ready)
	{
		UCoreKitSaveParticipantComponent* Component = Participants.FindChecked(Record->Id).Get();
		if (!Component || !Component->CanRestoreSaveState(Record->State))
		{
			return { ECoreKitWorldSaveResult::ParticipantRejected, { Record->Id } };
		}
	}

	for (const FCoreKitParticipantRecord* Record : Ready)
	{
		if (UCoreKitSaveParticipantComponent* Component = Participants.FindChecked(Record->Id).Get())
		{
			Component->ApplySaveState(Record->State);
		}
		else
		{
			Report.ParticipantIds.Add(Record->Id);
		}
	}

	Report.Result = Report.ParticipantIds.IsEmpty() ? ECoreKitWorldSaveResult::Success : ECoreKitWorldSaveResult::SuccessWithMissing;
	return Report;
}
