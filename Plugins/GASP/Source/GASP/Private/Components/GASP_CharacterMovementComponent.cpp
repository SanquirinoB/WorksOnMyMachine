#include "Components/GASP_CharacterMovementComponent.h"
#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/GASP_DF_MovementSettings.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_CharacterMovementComponent)

UGASP_CharacterMovementComponent::UGASP_CharacterMovementComponent()
{
	SetNetworkMoveDataContainer(MoveDataContainer);

	bRunPhysicsWithNoController = true;
	bNetworkAlwaysReplicateTransformUpdateTimestamp = true;

	RotationRate = FRotator(0.f, -1.f, 0.f);
	BrakingFrictionFactor = 1.f;
	BrakingFriction = 0.f;
	BrakingDecelerationWalking = 1500.f;
	GroundFriction = 5.f;
	MinAnalogWalkSpeed = 150.f;
	MaxWalkSpeed = 500.f;
	MaxAcceleration = 800.f;
	PerchRadiusThreshold = 20.0f;
	bUseFlatBaseForFloorChecks = true;
	bUseSeparateBrakingFriction = true;
	bCanWalkOffLedgesWhenCrouching = true;
	SettingsContainerTag = ContainerTags::Locomotion;

	NavAgentProps.bCanCrouch = true;

	NavMovementProperties.bUseAccelerationForPaths = true;
	SetCrouchedHalfHeight(60.f);
}

void UGASP_CharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyDataDrivenSettings();
}

void FGASPCharacterNetworkMoveData::ClientFillNetworkMoveData(
	const FSavedMove_Character& Move, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(Move, MoveType);

	const auto& SavedMove{static_cast<const FGASPSavedMove&>(Move)};

	RotationMode = SavedMove.RotationMode;
	StanceMode = SavedMove.StanceMode;
	bRotationModeUpdate = SavedMove.bRotationModeUpdate;
	AllowedGait = SavedMove.AllowedGait;
}

bool FGASPCharacterNetworkMoveData::Serialize(UCharacterMovementComponent& Movement,
                                              FArchive& Archive, UPackageMap* Map,
                                              ENetworkMoveType MoveType)
{
	Super::Serialize(Movement, Archive, Map, MoveType);

	NetSerializeOptionalValue(Archive.IsSaving(), Archive, RotationMode, RotationTags::OrientToMovement.GetTag(), Map);
	NetSerializeOptionalValue(Archive.IsSaving(), Archive, StanceMode, StanceTags::Standing.GetTag(), Map);
	NetSerializeOptionalValue(Archive.IsSaving(), Archive, AllowedGait, GaitTags::Run.GetTag(), Map);

	return !Archive.IsError();
}

FGASPCharacterNetworkMoveDataContainer::FGASPCharacterNetworkMoveDataContainer()
{
	NewMoveData = &MoveData[0];
	PendingMoveData = &MoveData[1];
	OldMoveData = &MoveData[2];
}

bool FGASPSavedMove::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FGASPSavedMove* NewCombineMove{static_cast<FGASPSavedMove*>(NewMove.Get())};

	return RotationMode == NewCombineMove->RotationMode && AllowedGait == NewCombineMove->AllowedGait && StanceMode ==
		NewCombineMove->StanceMode && Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FGASPSavedMove::CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC,
                                 const FVector& OldStartLocation)
{
	const auto OriginalRotation{OldMove->StartRotation};
	const auto OriginalRelativeRotation{OldMove->StartAttachRelativeRotation};

	const auto* NewUpdatedComponent{InCharacter->GetCharacterMovement()->UpdatedComponent.Get()};

	auto* MutablePreviousMove{const_cast<FSavedMove_Character*>(OldMove)};

	MutablePreviousMove->StartRotation = NewUpdatedComponent->GetComponentRotation();
	MutablePreviousMove->StartAttachRelativeRotation = NewUpdatedComponent->GetRelativeRotation();

	Super::CombineWith(OldMove, InCharacter, PC, OldStartLocation);

	MutablePreviousMove->StartRotation = OriginalRotation;
	MutablePreviousMove->StartAttachRelativeRotation = OriginalRelativeRotation;
}

void FGASPSavedMove::Clear()
{
	Super::Clear();

	AllowedGait = GaitTags::Run;
	bRotationModeUpdate = false;
	RotationMode = RotationTags::OrientToMovement;
	StanceMode = StanceTags::Standing;
}

uint8 FGASPSavedMove::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bRotationModeUpdate)
	{
		Result |= FLAG_Custom_0;
	}

	return Result;
}

void FGASPSavedMove::SetMoveFor(ACharacter* C, float InDeltaTime, const FVector& NewAccel,
                                FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	auto* CharacterMovement{
		Cast<UGASP_CharacterMovementComponent>(C->GetCharacterMovement())
	};

	if (IsValid(CharacterMovement))
	{
		AllowedGait = CharacterMovement->AllowedGait;
		bRotationModeUpdate = CharacterMovement->bRotationModeUpdate;
		RotationMode = CharacterMovement->RotationMode;
		StanceMode = CharacterMovement->StanceMode;
	}
}

void FGASPSavedMove::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	auto* CharacterMovement{
		Cast<UGASP_CharacterMovementComponent>(C->GetCharacterMovement())
	};

	if (IsValid(CharacterMovement))
	{
		CharacterMovement->AllowedGait = AllowedGait;
		CharacterMovement->RotationMode = RotationMode;
		CharacterMovement->StanceMode = StanceMode;
		CharacterMovement->bRotationModeUpdate = bRotationModeUpdate;

		CharacterMovement->RefreshGaitSettings();
	}
}

FNetworkPredictionData_Client_Base::FNetworkPredictionData_Client_Base(
	const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Base::AllocateNewMove()
{
	return MakeShared<FGASPSavedMove>();
}

FNetworkPredictionData_Client* UGASP_CharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (!ClientPredictionData)
	{
		auto* MutableThis = const_cast<ThisClass*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Base(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = MaxSmoothNetUpdateDist;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = NoSmoothNetUpdateDist;
	}

	return ClientPredictionData;
}

void UGASP_CharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bRotationModeUpdate = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void UGASP_CharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode,
                                                            uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (IsMovingOnGround() || IsInAir())
	{
		RotationRate = FRotator(0.f, IsInAir() ? InAirRotationYaw : -1.f, 0.f);
	}
}

void UGASP_CharacterMovementComponent::PhysNavWalking(float DeltaTime, int32 Iterations)
{
	if (GaitSettings.GetMovementCurve())
	{
		GroundFriction = GaitSettings.GetMovementCurve()->GetVectorValue(GetMappedSpeed()).Z;
	}

	RefreshGroundedSettings();

	Super::PhysNavWalking(DeltaTime, Iterations);
}

void UGASP_CharacterMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	if (GaitSettings.GetMovementCurve())
	{
		GroundFriction = GaitSettings.GetMovementCurve()->GetVectorValue(GetMappedSpeed()).Z;
	}

	RefreshGroundedSettings();

	Super::PhysWalking(DeltaTime, Iterations);
}

void UGASP_CharacterMovementComponent::MoveSmooth(const FVector& InVelocity, const float DeltaSeconds,
                                                 FStepDownResult* OutStepDownResult)
{
	if (IsMovingOnGround())
	{
		RefreshGroundedSettings();
	}

	Super::MoveSmooth(InVelocity, DeltaSeconds, OutStepDownResult);
}

void UGASP_CharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags,
                                                     const FVector& NewAccel)
{
	if (const auto* MoveData{static_cast<FGASPCharacterNetworkMoveData*>(GetCurrentNetworkMoveData())})
	{
		RotationMode = MoveData->RotationMode;
		bRotationModeUpdate = MoveData->bRotationModeUpdate;
		AllowedGait = MoveData->AllowedGait;
		StanceMode = MoveData->StanceMode;

		RefreshGaitSettings();
	}

	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

bool UGASP_CharacterMovementComponent::HasMovementInputVector() const
{
	return !GetPendingInputVector().IsZero();
}

void UGASP_CharacterMovementComponent::UpdateRotationMode()
{
	if (RotationMode == RotationTags::OrientToMovement)
	{
		bUseControllerDesiredRotation = false;
		bOrientRotationToMovement = true;
		return;
	}

	bUseControllerDesiredRotation = true;
	bOrientRotationToMovement = false;
}

void UGASP_CharacterMovementComponent::SetGait(const FGameplayTag& NewGait)
{
	if (AllowedGait != NewGait)
	{
		AllowedGait = NewGait;
	}
}

void UGASP_CharacterMovementComponent::SetStance(const FGameplayTag& NewStance)
{
	if (StanceMode != NewStance)
	{
		StanceMode = NewStance;

		RefreshGaitSettings();
	}
}

void UGASP_CharacterMovementComponent::RefreshGroundedSettings()
{
	if (bRotationModeUpdate)
	{
		UpdateRotationMode();
		bRotationModeUpdate = false;
	}

	MaxWalkSpeedCrouched = MaxWalkSpeed = GaitSettings.GetSpeed(AllowedGait, Velocity, GetLastUpdateRotation()) *
		SpeedMultiplier;
}

void UGASP_CharacterMovementComponent::SetRotationMode(const FGameplayTag& NewRotationMode)
{
	if (RotationMode == NewRotationMode)
	{
		return;
	}

	RotationMode = NewRotationMode;
	bRotationModeUpdate = true;

	if (!PawnOwner->HasAuthority())
	{
		UpdateRotationMode();
	}
}

float UGASP_CharacterMovementComponent::GetMaxAcceleration() const
{
	if (!IsMovingOnGround() || !IsValid(GaitSettings.GetMovementCurve()))
	{
		return Super::GetMaxAcceleration();
	}

	return GaitSettings.GetMovementCurve()->GetVectorValue(GetMappedSpeed()).X;
}

float UGASP_CharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	if (!IsMovingOnGround() || !IsValid(GaitSettings.GetMovementCurve()))
	{
		return Super::GetMaxBrakingDeceleration();
	}

	return GaitSettings.GetMovementCurve()->GetVectorValue(GetMappedSpeed()).Y;
}

float UGASP_CharacterMovementComponent::GetMappedSpeed() const
{
	float WalkSpeed = GaitSettings.GetSpeed(GaitTags::Walk, Velocity, GetLastUpdateRotation());
	float RunSpeed = GaitSettings.GetSpeed(GaitTags::Run, Velocity, GetLastUpdateRotation());
	float SprintSpeed = GaitSettings.GetSpeed(GaitTags::Sprint, Velocity, GetLastUpdateRotation());

	const auto Speed{UE_REAL_TO_FLOAT(Velocity.Size2D())};

	if (Speed > RunSpeed)
	{
		return FMath::GetMappedRangeValueClamped<float, float>({RunSpeed, SprintSpeed}, {2.0f, 3.0f}, Speed);
	}
	if (Speed > WalkSpeed)
	{
		return FMath::GetMappedRangeValueClamped<float, float>({WalkSpeed, RunSpeed}, {1.0f, 2.0f}, Speed);
	}

	return FMath::GetMappedRangeValueClamped<float, float>({0.0f, WalkSpeed}, {0.0f, 1.0f}, Speed);
}

bool UGASP_CharacterMovementComponent::IsInAir() const
{
	return IsFalling() | IsFlying();
}

void UGASP_CharacterMovementComponent::RefreshGaitSettings()
{
	GaitSettings = MovementSettings.Contains(StanceMode) ? MovementSettings.FindRef(StanceMode) : FGaitSettings();
}

void UGASP_CharacterMovementComponent::ApplyDataDrivenSettings()
{
	const UDataContainer* SettingsContainer = GetSettingsContainer();
	if (!IsValid(SettingsContainer))
	{
		RefreshGaitSettings();
		return;
	}

	const UGASP_DF_MovementSettings* Settings = SettingsContainer->GetDataFragmentByClass<UGASP_DF_MovementSettings>();
	if (!IsValid(Settings))
	{
		RefreshGaitSettings();
		return;
	}

	InAirRotationYaw = Settings->InAirRotationYaw;
	SpeedMultiplier = Settings->SpeedMultiplier;
	BrakingFrictionFactor = Settings->BrakingFrictionFactor;
	BrakingFriction = Settings->BrakingFriction;
	BrakingDecelerationWalking = Settings->BrakingDecelerationWalking;
	GroundFriction = Settings->GroundFriction;
	MinAnalogWalkSpeed = Settings->MinAnalogWalkSpeed;
	MaxWalkSpeed = Settings->DefaultMaxWalkSpeed;
	MaxWalkSpeedCrouched = Settings->DefaultMaxWalkSpeed;
	MaxAcceleration = Settings->MaxAcceleration;
	PerchRadiusThreshold = Settings->PerchRadiusThreshold;
	bUseFlatBaseForFloorChecks = Settings->bUseFlatBaseForFloorChecks;
	bUseSeparateBrakingFriction = Settings->bUseSeparateBrakingFriction;
	bCanWalkOffLedgesWhenCrouching = Settings->bCanWalkOffLedgesWhenCrouching;
	NavAgentProps.bCanCrouch = Settings->bCanCrouch;
	NavMovementProperties.bUseAccelerationForPaths = Settings->bUseAccelerationForPaths;
	SetCrouchedHalfHeight(Settings->CrouchedHalfHeight);
	MaxSmoothNetUpdateDist = Settings->MaxSmoothNetUpdateDist;
	NoSmoothNetUpdateDist = Settings->NoSmoothNetUpdateDist;
	if (!Settings->MovementSettings.IsEmpty())
	{
		MovementSettings = Settings->MovementSettings;
	}

	RefreshGaitSettings();
}

UDataDrivenComponent* UGASP_CharacterMovementComponent::GetDataDrivenComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDataDrivenComponent>() : nullptr;
}

const UDataContainer* UGASP_CharacterMovementComponent::GetSettingsContainer() const
{
	UDataDrivenComponent* DataDrivenComponent = GetDataDrivenComponent();
	return DataDrivenComponent ? DataDrivenComponent->GetDataContainer(SettingsContainerTag) : nullptr;
}


