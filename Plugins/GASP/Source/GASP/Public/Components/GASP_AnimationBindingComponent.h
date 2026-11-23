#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GASP_AnimationBindingComponent.generated.h"

class UChooserTable;
class UGASP_StateComponent;
class UDataContainer;
class UDataDrivenComponent;

UCLASS(ClassGroup=(GASP), meta=(BlueprintSpawnableComponent))
class GASP_API UGASP_AnimationBindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASP_AnimationBindingComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "GASP|Animation")
	void ApplyDataDrivenSettings();

	UFUNCTION(BlueprintCallable, Category = "GASP|Animation")
	void LinkAnimInstance(const UChooserTable* DataTable, const FGameplayTag OldState, const FGameplayTag State);

	FORCEINLINE const FGameplayTagContainer& GetStateContainer() const { return StateContainer; }

	UFUNCTION(BlueprintPure, Category = "GASP|Animation")
	FORCEINLINE UChooserTable* GetOverlayTable() const { return OverlayTable; }

	UFUNCTION(BlueprintPure, Category = "GASP|Animation")
	FORCEINLINE UChooserTable* GetPosesTable() const { return PosesTable; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Config")
	FGameplayTag SettingsContainerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Animation")
	TObjectPtr<UChooserTable> OverlayTable{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Animation")
	TObjectPtr<UChooserTable> PosesTable{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chooser")
	FGameplayTagContainer StateContainer{};

	UFUNCTION()
	void HandleOverlayModeChanged(FGameplayTag OldOverlayMode);

	UFUNCTION()
	void HandlePoseModeChanged(FGameplayTag OldPoseMode);

private:
	UDataDrivenComponent* GetDataDrivenComponent() const;
	const UDataContainer* GetSettingsContainer() const;

	UPROPERTY(Transient)
	TObjectPtr<UGASP_StateComponent> StateComponent{nullptr};
};


