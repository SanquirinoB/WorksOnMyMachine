#include "Components/GASP_AnimationBindingComponent.h"

#include "ChooserFunctionLibrary.h"
#include "Components/GASP_StateComponent.h"
#include "DataDriven/DataContainer.h"
#include "DataDriven/DataDrivenComponent.h"
#include "DataDriven/GASP_DF_AnimationBindingSettings.h"
#include "Types/TagTypes.h"
#include "Utils/GASPLinkedAnimInstanceSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASP_AnimationBindingComponent)

UGASP_AnimationBindingComponent::UGASP_AnimationBindingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SettingsContainerTag = ContainerTags::Animation;
}

void UGASP_AnimationBindingComponent::BeginPlay()
{
	Super::BeginPlay();

	StateComponent = GetOwner() ? GetOwner()->FindComponentByClass<UGASP_StateComponent>() : nullptr;
	ApplyDataDrivenSettings();

	if (!IsValid(StateComponent))
	{
		return;
	}

	StateContainer.Reset();
	if (StateComponent->GetOverlayMode().IsValid())
	{
		StateContainer.AddLeafTag(StateComponent->GetOverlayMode());
	}
	if (StateComponent->GetPoseMode().IsValid())
	{
		StateContainer.AddLeafTag(StateComponent->GetPoseMode());
	}

	StateComponent->OverlayModeChanged.AddUniqueDynamic(this, &ThisClass::HandleOverlayModeChanged);
	StateComponent->PoseModeChanged.AddUniqueDynamic(this, &ThisClass::HandlePoseModeChanged);
}

void UGASP_AnimationBindingComponent::ApplyDataDrivenSettings()
{
	const UDataContainer* SettingsContainer = GetSettingsContainer();
	if (!IsValid(SettingsContainer))
	{
		return;
	}

	const UGASP_DF_AnimationBindingSettings* Settings = SettingsContainer->GetDataFragmentByClass<UGASP_DF_AnimationBindingSettings>();
	if (!IsValid(Settings))
	{
		return;
	}

	OverlayTable = Settings->OverlayTable;
	PosesTable = Settings->PosesTable;
}

void UGASP_AnimationBindingComponent::LinkAnimInstance(const UChooserTable* DataTable, const FGameplayTag OldState,
                                                       const FGameplayTag State)
{
	if (!DataTable)
	{
		return;
	}

	StateContainer.RemoveTag(OldState);
	StateContainer.AddLeafTag(State);

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	if (const UGASPLinkedAnimInstanceSet* DataAsset = Cast<UGASPLinkedAnimInstanceSet>(
		UChooserFunctionLibrary::EvaluateChooser(this, DataTable, UGASPLinkedAnimInstanceSet::StaticClass())); IsValid(DataAsset))
	{
		MeshComponent->LinkAnimClassLayers(DataAsset->GetAnimInstance());
	}
}

void UGASP_AnimationBindingComponent::HandleOverlayModeChanged(FGameplayTag OldOverlayMode)
{
	LinkAnimInstance(OverlayTable, OldOverlayMode, StateComponent->GetOverlayMode());
}

void UGASP_AnimationBindingComponent::HandlePoseModeChanged(FGameplayTag OldPoseMode)
{
	LinkAnimInstance(PosesTable, OldPoseMode, StateComponent->GetPoseMode());
}

UDataDrivenComponent* UGASP_AnimationBindingComponent::GetDataDrivenComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UDataDrivenComponent>() : nullptr;
}

const UDataContainer* UGASP_AnimationBindingComponent::GetSettingsContainer() const
{
	UDataDrivenComponent* DataDrivenComponent = GetDataDrivenComponent();
	return DataDrivenComponent ? DataDrivenComponent->GetDataContainer(SettingsContainerTag) : nullptr;
}
