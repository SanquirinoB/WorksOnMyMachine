#pragma once

#include "CoreMinimal.h"
#include "DataDriven/DataFragmentBase.h"
#include "Engine/DataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "DataContainer.generated.h"

/** Data asset composed from one or more instanced data fragments. */
UCLASS(BlueprintType, Blueprintable)
class DATADRIVEN_API UDataContainer : public UDataAsset
{
	GENERATED_BODY()

public:
	#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	#endif

	/** Returns the first fragment compatible with T, preserving authored array order. */
	template <class T>
	T* GetDataFragmentByClass() const;

	/** Returns every fragment compatible with T, preserving authored array order. */
	template <class T>
	TArray<T*> GetDataFragmentsByClass() const;

	const TArray<TObjectPtr<UDataFragmentBase>>& GetAllDataFragments() const;

	UFUNCTION(BlueprintPure, Category = "DataDriven", DisplayName = "Get All Data Fragments")
	TArray<UDataFragmentBase*> BP_GetAllDataFragments() const;

protected:
	/** Designer-authored fragment list. Runtime systems should treat it as immutable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataDriven", Instanced)
	TArray<TObjectPtr<UDataFragmentBase>> DataFragments;
};

template <class T>
T* UDataContainer::GetDataFragmentByClass() const
{
	if (const UClass* TargetClass = T::StaticClass())
	{
		for (const TObjectPtr<UDataFragmentBase>& DataFragment : DataFragments)
		{
			if (IsValid(DataFragment) && DataFragment->IsA(TargetClass))
			{
				return Cast<T>(DataFragment);
			}
		}
	}

	return nullptr;
}

template <class T>
TArray<T*> UDataContainer::GetDataFragmentsByClass() const
{
	TArray<T*> Result;

	if (const UClass* TargetClass = T::StaticClass())
	{
		for (const TObjectPtr<UDataFragmentBase>& DataFragment : DataFragments)
		{
			if (IsValid(DataFragment) && DataFragment->IsA(TargetClass))
			{
				if (T* TypedFragment = Cast<T>(DataFragment))
				{
					Result.Add(TypedFragment);
				}
			}
		}
	}

	return Result;
}
