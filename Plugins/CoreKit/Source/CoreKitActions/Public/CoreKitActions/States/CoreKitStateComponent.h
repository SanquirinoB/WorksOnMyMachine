#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagAssetInterface.h"
#include "CoreKitActions/States/CoreKitStateTypes.h"
#include "CoreKitStateComponent.generated.h"

class UCoreKitActionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitStateSnapshotEvent, const FCoreKitStateSnapshot&,
                                            Snapshot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitStateReportEvent, const FCoreKitStateReport&, Report);

DECLARE_MULTICAST_DELEGATE(FCoreKitStatesSettled);

/** Local single-player authority for independent state domains. No automatic legacy cutover. */
UCLASS(ClassGroup=(AF), meta=(BlueprintSpawnableComponent))
class COREKITACTIONS_API UCoreKitStateComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UCoreKitStateComponent();
	UPROPERTY(BlueprintAssignable, Category="CoreKit Actions|States")
	FCoreKitStateSnapshotEvent OnSnapshotCommitted;
	UPROPERTY(BlueprintAssignable, Category="CoreKit Actions|States|Debug")
	FCoreKitStateReportEvent OnTransitionReported;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit Actions|States|Debug",
		meta=(ClampMin="0", ClampMax="256"))
	int32 HistoryLimit = 64;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit Actions|States|Debug")
	bool bLogTransitions = false;

	UFUNCTION(BlueprintCallable, Category="CoreKit Actions|States")
	FCoreKitStateReport RegisterDomain(const FCoreKitStateDomainDefinition& Definition, UObject* Writer,
	                                          FCoreKitStateAuthority& Authority);
	UFUNCTION(BlueprintCallable, Category="CoreKit Actions|States")
	FCoreKitStateReport RegisterProvidedDomain(const FCoreKitStateDomainDefinition& Definition,
	                                                  UObject* Provider, FCoreKitStateAuthority& Authority);
	UFUNCTION(BlueprintCallable, Category="CoreKit Actions|States")
	FCoreKitStateReport UnregisterDomain(const FCoreKitStateAuthority& Authority, UObject* Writer);
	UFUNCTION(BlueprintCallable, Category="CoreKit Actions|States")
	FCoreKitStateReport RefreshProvidedDomain(const FCoreKitStateAuthority& Authority, UObject* Provider);
	UFUNCTION(BlueprintCallable, Category="CoreKit Actions|States")
	FCoreKitStateReport TryTransition(const FCoreKitStateTransitionRequest& Request);
	/** All requests succeed together or none change state; guards read the same pre-transition snapshot. */
	UFUNCTION(BlueprintCallable, Category="CoreKit Actions|States")
	FCoreKitStateReport TryTransitionBatch(const TArray<FCoreKitStateTransitionRequest>& Requests);
	/** Pure comparison; never changes state, history, claims or presentation. */
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	FCoreKitStateReport EvaluateTransition(const FCoreKitStateTransitionRequest& Request) const;
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	FCoreKitStateSnapshot GetStateSnapshot() const;
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	FCoreKitStateDomainSnapshot GetDomainState(FName Domain) const;
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States|Debug")
	TArray<FCoreKitStateReport> GetTransitionHistory() const { return History; }

	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States|Debug")
	bool IsHealthy(FString& Reason) const;
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	bool IsTagReserved(FGameplayTag Tag) const;
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	bool IsCommittingTransition() const { return bPublishing || ProviderQueryDepth > 0; }

	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	int64 GetRevision() const { return Revision; }

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& Tags) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Optional integration, same actor only. Rejects attachment with conflicting legacy state claims. */
	bool BindActions(UCoreKitActionComponent* Actions);
	TArray<FString> GetStateSources(FGameplayTag Tag) const;
	FCoreKitStatesSettled OnStatesSettled;

private:
	struct FDomain
	{
		FCoreKitStateDomainDefinition Definition;
		TWeakObjectPtr<UObject> Writer;
		FGuid Generation;
		FCoreKitStateDomainSnapshot Snapshot;
	};

	FCoreKitStateReport RegisterInternal(const FCoreKitStateDomainDefinition& Definition, UObject* Writer,
	                                            FCoreKitStateAuthority& Authority, bool bProvider);
	ECoreKitStateResult CheckAuthority(const FCoreKitStateAuthority& Authority, UObject* Writer) const;
	FCoreKitStateReport MakeReport(ECoreKitStateResult Result, FName Domain,
	                                      const FString& Detail = FString()) const;
	void Publish(const TArray<FCoreKitStateReport>& Reports, bool bChanged);
	void Record(const FCoreKitStateReport& Report);
	double Now() const;
	TMap<FName, FDomain> Domains;
	TWeakObjectPtr<UCoreKitActionComponent> BoundActions;
	int64 Revision = 0;
	bool bPublishing = false;
	bool bEndingPlay = false;
	mutable int32 ProviderQueryDepth = 0;
	UPROPERTY(Transient)
	TArray<FCoreKitStateReport> History;
};
