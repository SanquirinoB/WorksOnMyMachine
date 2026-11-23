#pragma once

#include "CoreMinimal.h"
#include "CoreKitActorComponentContext.generated.h"

class AActor;
class UActorComponent;
class UInterface;

/** Result of resolving a component from an action owner's cached component context. */
UENUM(BlueprintType)
enum class ECoreKitActorComponentLookupResult : uint8
{
	Found,
	OwnerUnavailable,
	InvalidType,
	NotFound,
	Ambiguous
};

/**
 * Weak, refreshable snapshot of an actor's components. It has no knowledge of any project domain.
 * A query with more than one match is rejected until the caller supplies a component name.
 */
USTRUCT(BlueprintType)
struct COREKITACTIONS_API FCoreKitActorComponentContext
{
	GENERATED_BODY()

	void Refresh(AActor* InOwner);
	void Reset();
	bool IsPrepared() const { return bPrepared; }

	UActorComponent* FindByClass(TSubclassOf<UActorComponent> ComponentClass,
		ECoreKitActorComponentLookupResult& OutResult, FName ComponentName = NAME_None) const;
	UActorComponent* FindByInterface(TSubclassOf<UInterface> InterfaceClass,
		ECoreKitActorComponentLookupResult& OutResult, FName ComponentName = NAME_None) const;

	int32 GetRevision() const { return Revision; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> Owner;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UActorComponent>> Components;

	UPROPERTY(Transient)
	int32 Revision = 0;

	UPROPERTY(Transient)
	bool bPrepared = false;
};
