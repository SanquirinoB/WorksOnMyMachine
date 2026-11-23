#include "GASP_PlayerCharacter.h"
#include "Components/GASP_LocomotionComponent.h"
#include "Components/GASP_RagdollComponent.h"
#include "Components/GASP_StateComponent.h"
#include "Components/GASP_TraversalComponent.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "UObject/UnrealType.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_PlayerCharacter)

AGASP_PlayerCharacter::AGASP_PlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;

	GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));
	GameplayCamera->SetIsReplicated(true);

	if (GetMesh())
	{
		GameplayCamera->SetupAttachment(GetMesh(), NAME_None);
		GameplayCamera->SetRelativeLocation(FVector::ZAxisVector * 100.f);
	}
}

void AGASP_PlayerCharacter::PossessedBy(AController* NewController)
{
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		GameplayCamera->ActivateCameraForPlayerController(PC);
	}

	Super::PossessedBy(NewController);
}

void AGASP_PlayerCharacter::OnRep_Controller()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		GameplayCamera->ActivateCameraForPlayerController(PC);
	}

	Super::OnRep_Controller();
}

void AGASP_PlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (UGASP_StateComponent* GASPStateComponent = GetStateComponent())
	{
		GASPStateComponent->SetStanceMode(StanceTags::Crouching);
	}
}

void AGASP_PlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (UGASP_StateComponent* GASPStateComponent = GetStateComponent())
	{
		GASPStateComponent->SetStanceMode(StanceTags::Standing);
	}
}

void AGASP_PlayerCharacter::SprintAction(bool bPressed)
{
	if (UGASP_LocomotionComponent* GASPLocomotion = GetLocomotionComponent())
	{
		GASPLocomotion->RequestDesiredGait(bPressed ? GaitTags::Sprint : GaitTags::Run);
	}
}

void AGASP_PlayerCharacter::WalkAction(bool bPressed)
{
	const UGASP_StateComponent* GASPStateComponent = GetStateComponent();
	const FGameplayTag CurrentGait = GASPStateComponent ? GASPStateComponent->GetGait() : FGameplayTag::EmptyTag;
	if (UGASP_LocomotionComponent* GASPLocomotion = GetLocomotionComponent())
	{
		GASPLocomotion->RequestDesiredGait(CurrentGait != GaitTags::Walk ? GaitTags::Walk : GaitTags::Run);
	}
}

void AGASP_PlayerCharacter::CrouchAction(bool bPressed)
{
	const UGASP_StateComponent* GASPStateComponent = GetStateComponent();
	const FGameplayTag CurrentStance = GASPStateComponent ? GASPStateComponent->GetStanceMode() : FGameplayTag::EmptyTag;
	if (CurrentStance != StanceTags::Crouching)
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}
}

void AGASP_PlayerCharacter::JumpAction(bool bPressed)
{
	const UGASP_StateComponent* GASPStateComponent = GetStateComponent();
	const FGameplayTag CurrentLocomotionAction = GASPStateComponent
		? GASPStateComponent->GetLocomotionAction()
		: FGameplayTag::EmptyTag;
	if (CurrentLocomotionAction == LocomotionActionTags::Ragdoll)
	{
		if (UGASP_RagdollComponent* GASPRagdoll = GetRagdollComponent())
		{
			GASPRagdoll->StopRagdolling();
		}
		return;
	}

	if (bPressed && !IsDoingTraversal())
	{
		const FTraversalResult Result = TryTraversalAction();
		if ((Result.bTraversalCheckFailed || Result.bMontageSelectionFailed) && CanJump())
		{
			Jump();
		}
	}
	else
	{
		StopJumping();
	}
}

void AGASP_PlayerCharacter::AimAction(bool bPressed)
{
	if (UGASP_LocomotionComponent* GASPLocomotion = GetLocomotionComponent())
	{
		GASPLocomotion->RequestRotationMode(bPressed ? RotationTags::Aim : RotationTags::OrientToMovement);
	}
}

void AGASP_PlayerCharacter::RagdollAction(bool bPressed)
{
	if (bPressed)
	{
		if (UGASP_RagdollComponent* GASPRagdoll = GetRagdollComponent())
		{
			GASPRagdoll->StartRagdolling();
		}
	}
	else
	{
		if (UGASP_RagdollComponent* GASPRagdoll = GetRagdollComponent())
		{
			GASPRagdoll->StopRagdolling();
		}
	}
}

void AGASP_PlayerCharacter::StrafeAction(bool bPressed)
{
	const UGASP_StateComponent* GASPStateComponent = GetStateComponent();
	const FGameplayTag CurrentRotationMode = GASPStateComponent ? GASPStateComponent->GetRotationMode() : FGameplayTag::EmptyTag;
	if (UGASP_LocomotionComponent* GASPLocomotion = GetLocomotionComponent())
	{
		GASPLocomotion->RequestRotationMode(CurrentRotationMode != RotationTags::Strafe ? RotationTags::Strafe : RotationTags::OrientToMovement);
	}
}

void AGASP_PlayerCharacter::MoveAction(const FVector2D& Value)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector2D MovementInputScale = GetMovementInputScaleValue(Value);

	AddMovementInput(ForwardDirection, MovementInputScale.X);
	AddMovementInput(RightDirection, MovementInputScale.Y);
}

void AGASP_PlayerCharacter::LookAction(const FVector2D& Value)
{
	AddControllerYawInput(Value.X);
	AddControllerPitchInput(-Value.Y);
}

FTraversalResult AGASP_PlayerCharacter::TryTraversalAction() const
{
	if (UGASP_TraversalComponent* GASPTraversal = GetTraversalComponent())
	{
		return GASPTraversal->TryTraversalActionDefault();
	}

	return {true, false};
}

bool AGASP_PlayerCharacter::IsDoingTraversal() const
{
	const UGASP_TraversalComponent* GASPTraversal = GetTraversalComponent();
	return IsValid(GASPTraversal) && GASPTraversal->IsDoingTraversal();
}

FVector2D AGASP_PlayerCharacter::GetMovementInputScaleValue(const FVector2D& InVector) const
{
	if (const UGASP_LocomotionComponent* GASPLocomotion = GetLocomotionComponent())
	{
		return GASPLocomotion->GetMovementInputScaleValue(InVector);
	}

	return MovementStickMode > EAnalogStickBehaviorMode::FixedWalkRun ? InVector : InVector.GetSafeNormal();
}

FCameraProperties AGASP_PlayerCharacter::GetCameraProperties_Implementation()
{
	FCameraProperties Result;
	Result.ViewMode = EViewMode::ThirdPerson;

	if (const UGASP_StateComponent* GASPStateComponent = GetStateComponent())
	{
		Result.StanceMode = GASPStateComponent->GetStanceMode();
	}

	const UClass* Class = GetClass();

	auto ReadEnumByte = [this, Class](const TCHAR* PropertyName, uint8& OutValue) -> bool
	{
		if (const FProperty* Property = FindFProperty<FProperty>(Class, PropertyName))
		{
			if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
			{
				const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
				OutValue = static_cast<uint8>(UnderlyingProperty->GetUnsignedIntPropertyValue(
					UnderlyingProperty->ContainerPtrToValuePtr<void>(this)));
				return true;
			}

			if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
			{
				OutValue = ByteProperty->GetPropertyValue(ByteProperty->ContainerPtrToValuePtr<void>(this));
				return true;
			}
		}

		return false;
	};

	uint8 CameraModeValue = static_cast<uint8>(Result.CameraMode);
	if (ReadEnumByte(TEXT("CameraMode"), CameraModeValue))
	{
		Result.CameraMode = static_cast<ECameraMode>(CameraModeValue);
	}

	uint8 CameraStyleValue = static_cast<uint8>(Result.CameraStyle);
	if (ReadEnumByte(TEXT("Camera Style"), CameraStyleValue) || ReadEnumByte(TEXT("CameraStyle"), CameraStyleValue))
	{
		Result.CameraStyle = static_cast<ECameraStyle>(CameraStyleValue);
	}

	return Result;
}

