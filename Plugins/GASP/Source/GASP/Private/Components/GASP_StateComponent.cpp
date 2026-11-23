#include "Components/GASP_StateComponent.h"

#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/GASP_DF_StateDefaults.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_StateComponent)

UGASP_StateComponent::UGASP_StateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	DefaultsContainerTag = ContainerTags::Locomotion;

	DesiredGait = GaitTags::Run;
	Gait = GaitTags::Run;
	RotationMode = RotationTags::Strafe;
	MovementState = MovementStateTags::Idle;
	MovementMode = MovementModeTags::Grounded;
	StanceMode = StanceTags::Standing;
	OverlayMode = OverlayModeTags::Default;
	PoseMode = PoseModeTags::Default;
}

void UGASP_StateComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyDataDrivenDefaults();
}

void UGASP_StateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_SkipOwner;
	Parameters.RepNotifyCondition = REPNOTIFY_OnChanged;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredGait, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RotationMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MovementState, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MovementMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StanceMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LocomotionAction, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedAcceleration, Parameters);

	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PoseMode, Parameters);
}

void UGASP_StateComponent::ApplyDataDrivenDefaults()
{
	const UDataContainer* DefaultsContainer = GetDefaultsContainer();
	if (!IsValid(DefaultsContainer))
	{
		return;
	}

	const UGASP_DF_StateDefaults* Defaults = DefaultsContainer->GetDataFragmentByClass<UGASP_DF_StateDefaults>();
	if (!IsValid(Defaults))
	{
		return;
	}

	if (Defaults->DesiredGait.IsValid())
	{
		DesiredGait = Defaults->DesiredGait;
		Gait = DesiredGait;
	}
	if (Defaults->RotationMode.IsValid())
	{
		RotationMode = Defaults->RotationMode;
	}
	if (Defaults->MovementState.IsValid())
	{
		MovementState = Defaults->MovementState;
	}
	if (Defaults->MovementMode.IsValid())
	{
		MovementMode = Defaults->MovementMode;
	}
	if (Defaults->StanceMode.IsValid())
	{
		StanceMode = Defaults->StanceMode;
	}
	if (Defaults->OverlayMode.IsValid())
	{
		OverlayMode = Defaults->OverlayMode;
	}
	if (Defaults->PoseMode.IsValid())
	{
		PoseMode = Defaults->PoseMode;
	}
	if (Defaults->LocomotionAction.IsValid())
	{
		LocomotionAction = Defaults->LocomotionAction;
	}
}

void UGASP_StateComponent::SetDesiredGait(const FGameplayTag NewValue, bool bForce)
{
	if (DesiredGait != NewValue || bForce)
	{
		const FGameplayTag OldValue = DesiredGait;
		DesiredGait = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredGait, this);
		DesiredGaitChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetGait(const FGameplayTag NewValue, bool bForce)
{
	if (Gait != NewValue || bForce)
	{
		const FGameplayTag OldValue = Gait;
		Gait = NewValue;
		GaitChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetRotationMode(const FGameplayTag NewValue, bool bForce)
{
	if (RotationMode != NewValue || bForce)
	{
		const FGameplayTag OldValue = RotationMode;
		RotationMode = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RotationMode, this);
		RotationModeChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetMovementState(const FGameplayTag NewValue, bool bForce)
{
	if (MovementState != NewValue || bForce)
	{
		const FGameplayTag OldValue = MovementState;
		MovementState = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MovementState, this);
		MovementStateChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetMovementMode(const FGameplayTag NewValue, bool bForce)
{
	if (MovementMode != NewValue || bForce)
	{
		const FGameplayTag OldValue = MovementMode;
		MovementMode = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MovementMode, this);
		MovementModeChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetStanceMode(const FGameplayTag NewValue, bool bForce)
{
	if (StanceMode != NewValue || bForce)
	{
		const FGameplayTag OldValue = StanceMode;
		StanceMode = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StanceMode, this);
		StanceModeChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetOverlayMode(const FGameplayTag NewValue, bool bForce)
{
	if (OverlayMode != NewValue || bForce)
	{
		const FGameplayTag OldValue = OverlayMode;
		OverlayMode = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this);
		OverlayModeChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetPoseMode(const FGameplayTag NewValue, bool bForce)
{
	if (PoseMode != NewValue || bForce)
	{
		const FGameplayTag OldValue = PoseMode;
		PoseMode = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PoseMode, this);
		PoseModeChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetLocomotionAction(const FGameplayTag NewValue, bool bForce)
{
	if (LocomotionAction != NewValue || bForce)
	{
		const FGameplayTag OldValue = LocomotionAction;
		LocomotionAction = NewValue;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LocomotionAction, this);
		LocomotionActionChanged.Broadcast(OldValue);
	}
}

void UGASP_StateComponent::SetReplicatedAcceleration(const FVector& NewAcceleration)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, ReplicatedAcceleration, NewAcceleration, this);
}

void UGASP_StateComponent::OnRep_DesiredGait(const FGameplayTag& OldValue)
{
	DesiredGaitChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_RotationMode(const FGameplayTag& OldValue)
{
	RotationModeChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_MovementState(const FGameplayTag& OldValue)
{
	MovementStateChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_MovementMode(const FGameplayTag& OldValue)
{
	MovementModeChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_StanceMode(const FGameplayTag& OldValue)
{
	StanceModeChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_OverlayMode(const FGameplayTag& OldValue)
{
	OverlayModeChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_PoseMode(const FGameplayTag& OldValue)
{
	PoseModeChanged.Broadcast(OldValue);
}

void UGASP_StateComponent::OnRep_LocomotionAction(const FGameplayTag& OldValue)
{
	LocomotionActionChanged.Broadcast(OldValue);
}

UDataDrivenComponent* UGASP_StateComponent::GetDataDrivenComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDataDrivenComponent>() : nullptr;
}

const UDataContainer* UGASP_StateComponent::GetDefaultsContainer() const
{
	UDataDrivenComponent* DataDrivenComponent = GetDataDrivenComponent();
	return DataDrivenComponent ? DataDrivenComponent->GetDataContainer(DefaultsContainerTag) : nullptr;
}


