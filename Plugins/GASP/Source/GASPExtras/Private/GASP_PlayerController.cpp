#include "GASP_PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GASP_PlayerCharacter.h"

TSoftObjectPtr<UInputMappingContext> AGASP_PlayerController::GetDefaultInputMapping()
{
	return DefaultInputMapping;
}

AGASP_PlayerCharacter* AGASP_PlayerController::GetPossessedPlayer()
{
	return PossessedPlayer;
}

void AGASP_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PossessedPlayer = Cast<AGASP_PlayerCharacter>(InPawn);

	SetupInput();
}

void AGASP_PlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	PossessedPlayer = Cast<AGASP_PlayerCharacter>(GetPawn());

	SetupInput();
}

void AGASP_PlayerController::OnRep_Owner()
{
	Super::OnRep_Owner();

	PossessedPlayer = Cast<AGASP_PlayerCharacter>(GetOwner());

	SetupInput();
}

void AGASP_PlayerController::SetupInput() const
{
	if (!IsValid(PossessedPlayer))
	{
		return;
	}
	// Get the Enhanced Input Local Player Subsystem from the Local Player related to our Player Controller.
	if (UEnhancedInputLocalPlayerSubsystem* InputSystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// PawnClientRestart can run more than once in an Actor's lifetime, so start by clearing out any leftover
		// mappings.
		InputSystem->ClearAllMappings();

		FModifyContextOptions Options;
		Options.bForceImmediately = true;

		// Add each mapping context, along with their priority values. Higher values outprioritize lower values.
		InputSystem->AddMappingContext(DefaultInputMapping.LoadSynchronous(), 1, Options);
	}
}


