#include "Components/GASP_LocomotionComponent.h"

#include "Components/GASP_StateComponent.h"
#include "Components/GASP_CharacterMovementComponent.h"
#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/GASP_DF_LocomotionSettings.h"
#include "GameFramework/Character.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_LocomotionComponent)

UGASP_LocomotionComponent::UGASP_LocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SettingsContainerTag = ContainerTags::Locomotion;
}

void UGASP_LocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<UGASP_StateComponent>() : nullptr;
	if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
	{
		MovementComponent = Cast<UGASP_CharacterMovementComponent>(CharacterOwner->GetCharacterMovement());
	}

	ApplyDataDrivenSettings();
}

void UGASP_LocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshLocomotionState();
}

void UGASP_LocomotionComponent::ApplyDataDrivenSettings()
{
	const UDataContainer* SettingsContainer = GetSettingsContainer();
	if (!IsValid(SettingsContainer))
	{
		return;
	}

	const UGASP_DF_LocomotionSettings* Settings = SettingsContainer->GetDataFragmentByClass<UGASP_DF_LocomotionSettings>();
	if (!IsValid(Settings))
	{
		return;
	}

	AnalogMovementThreshold = Settings->AnalogMovementThreshold;
	MovementStickMode = Settings->MovementStickMode;
	SprintMaxDirectionAngle = Settings->SprintMaxDirectionAngle;
}

void UGASP_LocomotionComponent::RefreshLocomotionState()
{
	if (!IsValid(StateComponent) || !IsValid(MovementComponent))
	{
		return;
	}

	if (GetOwner()->GetLocalRole() >= ROLE_AutonomousProxy)
	{
		StateComponent->SetReplicatedAcceleration(MovementComponent->GetCurrentAcceleration());
	}

	const bool bIsMoving = !MovementComponent->Velocity.IsNearlyZero(0.1f) &&
		!StateComponent->GetReplicatedAcceleration().IsNearlyZero(10.0f);
	StateComponent->SetMovementState(bIsMoving ? MovementStateTags::Moving : MovementStateTags::Idle);

	switch (MovementComponent->MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
	case MOVE_Flying:
	case MOVE_None:
		StateComponent->SetMovementMode(MovementModeTags::Grounded);
		break;
	case MOVE_Falling:
		StateComponent->SetMovementMode(MovementModeTags::InAir);
		break;
	default:
		StateComponent->SetMovementMode(FGameplayTag::EmptyTag);
		break;
	}

	if (StateComponent->GetMovementMode() == MovementModeTags::Grounded &&
		StateComponent->GetStanceMode() != StanceTags::Crouching)
	{
		RefreshGait();
	}
}

void UGASP_LocomotionComponent::RefreshGait()
{
	if (!IsValid(StateComponent))
	{
		return;
	}

	FGameplayTag NewGait = StateComponent->GetDesiredGait();

	if (StateComponent->GetDesiredGait() == GaitTags::Sprint && CanSprint())
	{
		NewGait = HasFullMovementInput() ? GaitTags::Sprint : GaitTags::Run;
	}
	else if (StateComponent->GetDesiredGait() == GaitTags::Walk)
	{
		NewGait = GaitTags::Walk;
	}
	else if (StateComponent->GetDesiredGait() == GaitTags::Sprint || StateComponent->GetDesiredGait() == GaitTags::Run)
	{
		NewGait = HasFullMovementInput() ? GaitTags::Run : GaitTags::Walk;
	}

	StateComponent->SetGait(NewGait);

	if (IsValid(MovementComponent))
	{
		MovementComponent->SetGait(NewGait);
		MovementComponent->SetRotationMode(StateComponent->GetRotationMode());
		MovementComponent->SetStance(StateComponent->GetStanceMode());
	}
}

void UGASP_LocomotionComponent::RequestDesiredGait(const FGameplayTag NewGait, bool bForce)
{
	if (!IsValid(StateComponent))
	{
		return;
	}

	StateComponent->SetDesiredGait(NewGait, bForce);
	RefreshGait();

	if (const AActor* OwnerActor = GetOwner();
		IsValid(OwnerActor) && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy && IsValid(OwnerActor->GetNetConnection()))
	{
		ServerRequestDesiredGait(NewGait, bForce);
	}
}

void UGASP_LocomotionComponent::RequestRotationMode(const FGameplayTag NewRotationMode, bool bForce)
{
	if (!IsValid(StateComponent))
	{
		return;
	}

	StateComponent->SetRotationMode(NewRotationMode, bForce);
	if (IsValid(MovementComponent))
	{
		MovementComponent->SetRotationMode(NewRotationMode);
	}
	RefreshGait();

	if (const AActor* OwnerActor = GetOwner();
		IsValid(OwnerActor) && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy && IsValid(OwnerActor->GetNetConnection()))
	{
		ServerRequestRotationMode(NewRotationMode, bForce);
	}
}

bool UGASP_LocomotionComponent::CanSprint() const
{
	if (!IsValid(StateComponent))
	{
		return false;
	}

	if (StateComponent->GetRotationMode() == RotationTags::OrientToMovement)
	{
		return true;
	}
	if (StateComponent->GetRotationMode() == RotationTags::Aim)
	{
		return false;
	}

	const FVector ViewDirection = GetOwner() ? GetOwner()->GetActorForwardVector() : FVector::ForwardVector;
	const float Dot = FVector::DotProduct(StateComponent->GetReplicatedAcceleration().GetSafeNormal2D(),
	                                      ViewDirection.GetSafeNormal2D());
	return Dot > FMath::Cos(FMath::DegreesToRadians(SprintMaxDirectionAngle));
}

bool UGASP_LocomotionComponent::HasFullMovementInput() const
{
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	if (!IsValid(CharacterOwner))
	{
		return false;
	}

	if (MovementStickMode == EAnalogStickBehaviorMode::FixedWalkRun ||
		MovementStickMode == EAnalogStickBehaviorMode::VariableWalkRun)
	{
		return CharacterOwner->GetPendingMovementInputVector().Size2D() >= AnalogMovementThreshold;
	}

	return true;
}

FVector2D UGASP_LocomotionComponent::GetMovementInputScaleValue(const FVector2D InVector) const
{
	return MovementStickMode > EAnalogStickBehaviorMode::FixedWalkRun ? InVector : InVector.GetSafeNormal();
}

void UGASP_LocomotionComponent::ServerRequestDesiredGait_Implementation(const FGameplayTag NewGait, bool bForce)
{
	RequestDesiredGait(NewGait, bForce);
}

void UGASP_LocomotionComponent::ServerRequestRotationMode_Implementation(const FGameplayTag NewRotationMode, bool bForce)
{
	RequestRotationMode(NewRotationMode, bForce);
}

UDataDrivenComponent* UGASP_LocomotionComponent::GetDataDrivenComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDataDrivenComponent>() : nullptr;
}

const UDataContainer* UGASP_LocomotionComponent::GetSettingsContainer() const
{
	UDataDrivenComponent* DataDrivenComponent = GetDataDrivenComponent();
	return DataDrivenComponent ? DataDrivenComponent->GetDataContainer(SettingsContainerTag) : nullptr;
}


