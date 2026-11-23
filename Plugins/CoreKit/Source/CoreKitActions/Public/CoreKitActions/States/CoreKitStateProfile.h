#pragma once

#include "Engine/DataAsset.h"
#include "CoreKitActions/States/CoreKitStateTypes.h"
#include "CoreKitStateProfile.generated.h"

/** Authored domains. A domain authority must be acquired before any state is activated. */
UCLASS(BlueprintType)
class COREKITACTIONS_API UCoreKitStateProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="States")
	TArray<FCoreKitStateDomainDefinition> Domains;
	UFUNCTION(BlueprintPure, Category="CoreKit Actions|States")
	bool ValidateProfile(FString& Reason) const;
};
