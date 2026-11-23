#include "Actors/GASP_Character.h"

#include "Components/GASP_StateComponent.h"
#include "Components/GASP_AnimationBindingComponent.h"
#include "Components/GASP_LocomotionComponent.h"
#include "Components/GASP_RagdollComponent.h"
#include "Components/GASP_CharacterMovementComponent.h"
#include "Components/GASP_TraversalComponent.h"
#include "DataDriven/DataDrivenComponent.h"
#include "MotionWarpingComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_Character)

AGASP_Character::AGASP_Character(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGASP_CharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;

	SetReplicates(true);
	SetReplicatingMovement(true);

	if (GetMesh())
	{
		GetMesh()->SetRelativeRotation_Direct({0.f, -90.f, 0.f});
		GetMesh()->SetRelativeLocation_Direct({0.f, 0.f, -90.f});
	}

	DataDrivenComponent = CreateDefaultSubobject<UDataDrivenComponent>(TEXT("DataDrivenComponent"));
	StateComponent = CreateDefaultSubobject<UGASP_StateComponent>(TEXT("StateComponent"));
	LocomotionComponent = CreateDefaultSubobject<UGASP_LocomotionComponent>(TEXT("LocomotionComponent"));
	AnimationBindingComponent = CreateDefaultSubobject<UGASP_AnimationBindingComponent>(TEXT("AnimationBindingComponent"));
	RagdollComponent = CreateDefaultSubobject<UGASP_RagdollComponent>(TEXT("RagdollComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	TraversalComponent = CreateDefaultSubobject<UGASP_TraversalComponent>(TEXT("TraversalComponent"));

	DataDrivenComponent->bEditableWhenInherited = true;
	StateComponent->bEditableWhenInherited = true;
	LocomotionComponent->bEditableWhenInherited = true;
	AnimationBindingComponent->bEditableWhenInherited = true;
	RagdollComponent->bEditableWhenInherited = true;
	MotionWarpingComponent->bEditableWhenInherited = true;
	TraversalComponent->bEditableWhenInherited = true;
}

void AGASP_Character::BeginPlay()
{
	Super::BeginPlay();
}


