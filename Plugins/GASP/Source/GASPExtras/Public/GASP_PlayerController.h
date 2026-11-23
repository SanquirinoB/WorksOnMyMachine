#pragma once

#include "GameFramework/PlayerController.h"
#include "GASP_PlayerController.generated.h"

/**
 *
 */
UCLASS()
class GASPEXTRAS_API AGASP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGASP_PlayerController() = default;

	UFUNCTION(BlueprintPure)
	class AGASP_PlayerCharacter* GetPossessedPlayer();
	UFUNCTION(BlueprintPure)
	TSoftObjectPtr<class UInputMappingContext> GetDefaultInputMapping();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void OnRep_Owner() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, BlueprintGetter=GetPossessedPlayer)
	TObjectPtr<AGASP_PlayerCharacter> PossessedPlayer{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintGetter=GetDefaultInputMapping)
	TSoftObjectPtr<UInputMappingContext> DefaultInputMapping{};

	void SetupInput() const;
};


