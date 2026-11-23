#include "Components/GASP_RagdollComponent.h"

#include "Animation/GASPAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/GASP_StateComponent.h"
#include "Components/GASP_CharacterMovementComponent.h"
#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/GASP_DF_RagdollSettings.h"
#include "GameFramework/Character.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_RagdollComponent)

static const FName NAME_pelvis(TEXT("pelvis"));
static const FName NAME_spine_03(TEXT("spine_03"));

UGASP_RagdollComponent::UGASP_RagdollComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SettingsContainerTag = ContainerTags::Ragdoll;
}

void UGASP_RagdollComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheOwnerDependencies();
	ApplyDataDrivenSettings();
}

void UGASP_RagdollComponent::CacheOwnerDependencies()
{
	CharacterOwner = Cast<ACharacter>(GetOwner());
	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<UGASP_StateComponent>() : nullptr;
	MovementComponent = GetOwner() ? GetOwner()->FindComponentByClass<UGASP_CharacterMovementComponent>() : nullptr;
	MeshComponent = CharacterOwner.IsValid() ? CharacterOwner->GetMesh() : nullptr;
	CapsuleComponent = CharacterOwner.IsValid() ? CharacterOwner->GetCapsuleComponent() : nullptr;
}

void UGASP_RagdollComponent::ApplyDataDrivenSettings()
{
	const UDataContainer* SettingsContainer = GetSettingsContainer();
	if (!IsValid(SettingsContainer))
	{
		return;
	}

	const UGASP_DF_RagdollSettings* Settings = SettingsContainer->GetDataFragmentByClass<UGASP_DF_RagdollSettings>();
	if (!IsValid(Settings))
	{
		return;
	}

	GetUpMontageFront = Settings->GetUpMontageFront;
	GetUpMontageBack = Settings->GetUpMontageBack;
	bLimitInitialRagdollSpeed = Settings->bLimitInitialRagdollSpeed;
	MontageBlendOutDuration = Settings->MontageBlendOutDuration;
	MinInitialSpeedLimit = Settings->MinInitialSpeedLimit;
	PullForce = Settings->PullForce;
	PullForceInterpolationSpeed = Settings->PullForceInterpolationSpeed;
	PullForceMinDistance = Settings->PullForceMinDistance;
	PullForceMaxDistance = Settings->PullForceMaxDistance;
	SpinePullSpeedThreshold = Settings->SpinePullSpeedThreshold;
	MotorReferenceSpeed = Settings->MotorReferenceSpeed;
	MotorStiffness = Settings->MotorStiffness;
	FlailPlayRateReferenceSpeed = Settings->FlailPlayRateReferenceSpeed;
}

void UGASP_RagdollComponent::StartRagdolling()
{
	if (!CharacterOwner.IsValid() || CharacterOwner->GetLocalRole() <= ROLE_SimulatedProxy || !IsRagdollingAllowedToStart())
	{
		return;
	}

	if (CharacterOwner->GetLocalRole() >= ROLE_Authority)
	{
		MulticastStartRagdolling();
	}
	else
	{
		if (MovementComponent.IsValid())
		{
			MovementComponent->FlushServerMoves();
		}
		ServerStartRagdolling();
	}
}

bool UGASP_RagdollComponent::StopRagdolling()
{
	if (!CharacterOwner.IsValid() || CharacterOwner->GetLocalRole() <= ROLE_SimulatedProxy || !IsRagdollingAllowedToStop())
	{
		return false;
	}

	if (CharacterOwner->GetLocalRole() >= ROLE_Authority)
	{
		MulticastStopRagdolling();
	}
	else
	{
		ServerStopRagdolling();
	}

	return true;
}

void UGASP_RagdollComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!StateComponent.IsValid() || StateComponent->GetLocomotionAction() != LocomotionActionTags::Ragdoll)
	{
		return;
	}

	RefreshRagdolling(DeltaTime);
}

void UGASP_RagdollComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollTargetLocation, Parameters);
}

bool UGASP_RagdollComponent::IsRagdollingAllowedToStart() const
{
	return CharacterOwner.IsValid() && StateComponent.IsValid() && MeshComponent.IsValid() &&
		StateComponent->GetLocomotionAction() != LocomotionActionTags::Ragdoll &&
		MeshComponent->GetBodyInstance(NAME_pelvis) != nullptr &&
		MeshComponent->GetBodyInstance(NAME_spine_03) != nullptr;
}

bool UGASP_RagdollComponent::IsRagdollingAllowedToStop() const
{
	return CharacterOwner.IsValid() && StateComponent.IsValid() && MeshComponent.IsValid() &&
		StateComponent->GetLocomotionAction() == LocomotionActionTags::Ragdoll &&
		MeshComponent->GetBodyInstance(NAME_pelvis) != nullptr &&
		MeshComponent->GetBodyInstance(NAME_spine_03) != nullptr;
}

void UGASP_RagdollComponent::StartRagdollingImplementation()
{
	if (!CharacterOwner.IsValid() || !StateComponent.IsValid() || !MovementComponent.IsValid() || !MeshComponent.IsValid() ||
		!CapsuleComponent.IsValid() || !IsRagdollingAllowedToStart())
	{
		return;
	}

	MeshComponent->bUpdateJointsFromAnimation = true;
	if (!MeshComponent->IsRunningParallelEvaluation() && MeshComponent->GetBoneSpaceTransforms().Num() > 0)
	{
		MeshComponent->UpdateRBJointMotors();
	}

	if (IsValid(MeshComponent->GetAnimInstance()))
	{
		MeshComponent->GetAnimInstance()->Montage_Stop(MontageBlendOutDuration);
	}

	MovementComponent->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;

	MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetSimulatePhysics(true);

	const FBodyInstance* PelvisBody = MeshComponent->GetBodyInstance(NAME_pelvis);
	FVector PelvisLocation = FVector::ZeroVector;
	FPhysicsCommand::ExecuteRead(PelvisBody->ActorHandle, [this, &PelvisLocation](const FPhysicsActorHandle& ActorHandle)
	{
		PelvisLocation = FPhysicsInterface::GetTransform_AssumesLocked(ActorHandle, true).GetLocation();
		RagdollingState.Velocity = FPhysicsInterface::GetLinearVelocity_AssumesLocked(ActorHandle);
	});

	RagdollingState.PullForce = 0.0f;
	if (bLimitInitialRagdollSpeed)
	{
		RagdollingState.SpeedLimitFrameTimeRemaining = 8;
		RagdollingState.SpeedLimit = FMath::Max(MinInitialSpeedLimit, UE_REAL_TO_FLOAT(CharacterOwner->GetVelocity().Size()));
		ConstraintRagdollSpeed();
	}

	if (CharacterOwner->GetLocalRole() >= ROLE_Authority)
	{
		SetRagdollTargetLocation(FVector::ZeroVector);
	}

	if (CharacterOwner->IsLocallyControlled() || (CharacterOwner->GetLocalRole() >= ROLE_Authority && !IsValid(CharacterOwner->GetController())))
	{
		SetRagdollTargetLocation(PelvisLocation);
	}

	MovementComponent->SetMovementMode(MOVE_None);
	StateComponent->SetLocomotionAction(LocomotionActionTags::Ragdoll);
	OnRagdollStarted.Broadcast();
	ReceiveRagdollStarted();
}

bool UGASP_RagdollComponent::StopRagdollingImplementation()
{
	UGASPAnimInstance* AnimationInstance = MeshComponent.IsValid()
		? Cast<UGASPAnimInstance>(MeshComponent->GetAnimInstance())
		: nullptr;
	if (!CharacterOwner.IsValid() || !StateComponent.IsValid() || !MovementComponent.IsValid() ||
		!MeshComponent.IsValid() || !CapsuleComponent.IsValid() || !IsValid(AnimationInstance) || !IsRagdollingAllowedToStop())
	{
		return false;
	}

	FPoseSnapshot& FinalRagdollPose = AnimationInstance->SnapshotFinalRagdollPose();
	const FTransform PelvisTransform = MeshComponent->GetSocketTransform(NAME_pelvis);
	const FRotator PelvisRotation = PelvisTransform.Rotator();

	MeshComponent->bUpdateJointsFromAnimation = false;
	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(ECC_Pawn);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	MovementComponent->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = false;

	bool bGrounded = false;
	const FVector NewActorLocation = RagdollTraceGround(bGrounded);
	const bool bRagdollFacingUpward = FMath::UnwindDegrees(PelvisRotation.Roll) <= 0.0f;

	FRotator NewActorRotation = CharacterOwner->GetActorRotation();
	NewActorRotation.Yaw = bRagdollFacingUpward ? PelvisRotation.Yaw - 180.0f : PelvisRotation.Yaw;
	CharacterOwner->SetActorLocationAndRotation(NewActorLocation, NewActorRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FTransform& ActorTransform = CharacterOwner->GetActorTransform();
	MeshComponent->SetWorldLocationAndRotationNoPhysics(
		ActorTransform.TransformPositionNoScale(CharacterOwner->GetBaseTranslationOffset()),
		ActorTransform.TransformRotation(CharacterOwner->GetBaseRotationOffset()).Rotator());
	MeshComponent->AttachToComponent(CapsuleComponent.Get(), FAttachmentTransformRules::KeepWorldTransform);

	if (MeshComponent->ShouldUseUpdateRateOptimizations())
	{
		MeshComponent->bEnableUpdateRateOptimizations = false;
		CharacterOwner->GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(CharacterOwner.Get(), [CharacterOwner = CharacterOwner.Get()]
		{
			if (IsValid(CharacterOwner) && IsValid(CharacterOwner->GetMesh()))
			{
				CharacterOwner->GetMesh()->bEnableUpdateRateOptimizations = true;
			}
		}));
	}

	const FReferenceSkeleton& ReferenceSkeleton = MeshComponent->GetSkinnedAsset()->GetRefSkeleton();
	const int32 PelvisBoneIndex = ReferenceSkeleton.FindBoneIndex(NAME_pelvis);
	if (PelvisBoneIndex >= 0)
	{
		FinalRagdollPose.LocalTransforms[PelvisBoneIndex] = PelvisTransform.GetRelativeTransform(MeshComponent->GetComponentTransform());
	}

	if (bGrounded)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		AnimationInstance->Montage_Play(SelectGetUpMontage(bRagdollFacingUpward));
	}
	else
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
		MovementComponent->Velocity = RagdollingState.Velocity;
	}

	StateComponent->SetLocomotionAction(FGameplayTag::EmptyTag);
	OnRagdollStopped.Broadcast();
	ReceiveRagdollStopped();
	return true;
}

void UGASP_RagdollComponent::ServerStartRagdolling_Implementation()
{
	if (IsRagdollingAllowedToStart())
	{
		MulticastStartRagdolling();
		if (AActor* OwnerActor = GetOwner())
		{
			OwnerActor->ForceNetUpdate();
		}
	}
}

void UGASP_RagdollComponent::MulticastStartRagdolling_Implementation()
{
	StartRagdollingImplementation();
}

void UGASP_RagdollComponent::ServerStopRagdolling_Implementation()
{
	if (IsRagdollingAllowedToStop())
	{
		MulticastStopRagdolling();
		if (AActor* OwnerActor = GetOwner())
		{
			OwnerActor->ForceNetUpdate();
		}
	}
}

void UGASP_RagdollComponent::MulticastStopRagdolling_Implementation()
{
	StopRagdollingImplementation();
}

void UGASP_RagdollComponent::SetRagdollTargetLocation(const FVector& NewTargetLocation)
{
	if (RagdollTargetLocation != NewTargetLocation)
	{
		RagdollTargetLocation = NewTargetLocation;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RagdollTargetLocation, this);
	}
}

const FRagdollingState& UGASP_RagdollComponent::GetRagdollingState() const
{
	return RagdollingState;
}

UAnimMontage* UGASP_RagdollComponent::SelectGetUpMontage(bool bRagdollFacingUpward) const
{
	return bRagdollFacingUpward ? GetUpMontageBack : GetUpMontageFront;
}

void UGASP_RagdollComponent::RefreshRagdolling(float DeltaTime)
{
	if (!CharacterOwner.IsValid() || !MovementComponent.IsValid() || !MeshComponent.IsValid() || !CapsuleComponent.IsValid())
	{
		return;
	}

	const FBodyInstance* PelvisBody = MeshComponent->GetBodyInstance(NAME_pelvis);
	if (!PelvisBody)
	{
		return;
	}

	FVector PelvisLocation = FVector::ZeroVector;
	FPhysicsCommand::ExecuteRead(PelvisBody->ActorHandle, [this, &PelvisLocation](const FPhysicsActorHandle& ActorHandle)
	{
		PelvisLocation = FPhysicsInterface::GetTransform_AssumesLocked(ActorHandle, true).GetLocation();
		RagdollingState.Velocity = FPhysicsInterface::GetLinearVelocity_AssumesLocked(ActorHandle);
	});

	const bool bLocallyControlled = CharacterOwner->IsLocallyControlled() ||
		(CharacterOwner->GetLocalRole() >= ROLE_Authority && !IsValid(CharacterOwner->GetController()));
	if (bLocallyControlled)
	{
		SetRagdollTargetLocation(PelvisLocation);
	}

	bool bGrounded = false;
	CharacterOwner->SetActorLocation(RagdollTraceGround(bGrounded), false);

	const FVector Location = CharacterOwner->GetActorLocation();
	MeshComponent->SetWorldLocation({Location.X, Location.Y, Location.Z - CapsuleComponent->GetScaledCapsuleHalfHeight()});

	if (!bLocallyControlled && !RagdollTargetLocation.IsZero())
	{
		RagdollingState.PullForce = FMath::FInterpTo(
			RagdollingState.PullForce, PullForce, DeltaTime, PullForceInterpolationSpeed);

		const double HorizontalSpeedSquared = RagdollingState.Velocity.SizeSquared2D();
		const FName PullForceBoneName = HorizontalSpeedSquared > FMath::Square(SpinePullSpeedThreshold)
			? NAME_spine_03
			: NAME_pelvis;
		FBodyInstance* PullForceBody = MeshComponent->GetBodyInstance(PullForceBoneName);

		FPhysicsCommand::ExecuteWrite(PullForceBody->ActorHandle, [this](const FPhysicsActorHandle& ActorHandle)
		{
			if (!FPhysicsInterface::IsRigidBody(ActorHandle))
			{
				return;
			}

			const FVector PullForceVector = RagdollTargetLocation - FPhysicsInterface::GetTransform_AssumesLocked(ActorHandle, true).GetLocation();
			if (PullForceVector.SizeSquared() > FMath::Square(PullForceMinDistance))
			{
				FPhysicsInterface::AddForce_AssumesLocked(
					ActorHandle,
					PullForceVector.GetClampedToMaxSize(PullForceMaxDistance) * RagdollingState.PullForce,
					true,
					true);
			}
		});
	}

	const float SpeedAmount = FMath::Clamp(
		UE_REAL_TO_FLOAT(RagdollingState.Velocity.Size() / MotorReferenceSpeed), 0.f, 1.f);
	MeshComponent->SetAllMotorsAngularDriveParams(SpeedAmount * MotorStiffness, 0.0f, 0.0f);

	if (RagdollingState.SpeedLimitFrameTimeRemaining > 0)
	{
		RagdollingState.SpeedLimitFrameTimeRemaining -= 1;
		ConstraintRagdollSpeed();
	}
}

FVector UGASP_RagdollComponent::RagdollTraceGround(bool& bGrounded) const
{
	if (!CharacterOwner.IsValid() || !MovementComponent.IsValid() || !CapsuleComponent.IsValid())
	{
		bGrounded = false;
		return FVector::ZeroVector;
	}

	const FVector CurrentRagdollLocation = !RagdollTargetLocation.IsZero() ? FVector{RagdollTargetLocation} : CharacterOwner->GetActorLocation();
	const float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();

	const FVector TraceStart{CurrentRagdollLocation.X, CurrentRagdollLocation.Y, CurrentRagdollLocation.Z + 2.0f * CapsuleRadius};
	const FVector TraceEnd{CurrentRagdollLocation.X, CurrentRagdollLocation.Y, CurrentRagdollLocation.Z - CapsuleHalfHeight + CapsuleRadius};
	const ECollisionChannel CollisionChannel = MovementComponent->UpdatedComponent->GetCollisionObjectType();

	FCollisionQueryParams QueryParameters{__FUNCTION__, false, CharacterOwner.Get()};
	FCollisionResponseParams CollisionResponses;
	MovementComponent->InitCollisionParams(QueryParameters, CollisionResponses);

	FHitResult Hit;
	bGrounded = CharacterOwner->GetWorld()->SweepSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		CollisionChannel,
		FCollisionShape::MakeSphere(CapsuleRadius),
		QueryParameters,
		CollisionResponses);

	return FVector{
		CurrentRagdollLocation.X,
		CurrentRagdollLocation.Y,
		bGrounded ? Hit.Location.Z + CapsuleHalfHeight - CapsuleRadius + UCharacterMovementComponent::MIN_FLOOR_DIST : CurrentRagdollLocation.Z
	};
}

void UGASP_RagdollComponent::ConstraintRagdollSpeed() const
{
	if (!MeshComponent.IsValid())
	{
		return;
	}

	MeshComponent->ForEachBodyBelow(NAME_None, true, false, [this](FBodyInstance* Body)
	{
		FPhysicsCommand::ExecuteWrite(Body->ActorHandle, [this](const FPhysicsActorHandle& ActorHandle)
		{
			if (!FPhysicsInterface::IsRigidBody(ActorHandle))
			{
				return;
			}

			FVector Velocity = FPhysicsInterface::GetLinearVelocity_AssumesLocked(ActorHandle);
			if (Velocity.SizeSquared() <= FMath::Square(RagdollingState.SpeedLimit))
			{
				return;
			}

			Velocity.Normalize();
			Velocity *= RagdollingState.SpeedLimit;
			FPhysicsInterface::SetLinearVelocity_AssumesLocked(ActorHandle, Velocity);
		});
	});
}

UDataDrivenComponent* UGASP_RagdollComponent::GetDataDrivenComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDataDrivenComponent>() : nullptr;
}

const UDataContainer* UGASP_RagdollComponent::GetSettingsContainer() const
{
	UDataDrivenComponent* DataDrivenComponent = GetDataDrivenComponent();
	return DataDrivenComponent ? DataDrivenComponent->GetDataContainer(SettingsContainerTag) : nullptr;
}


