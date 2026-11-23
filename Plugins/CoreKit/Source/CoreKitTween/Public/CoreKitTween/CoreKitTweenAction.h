#pragma once

#include "CoreKitTween/CoreKitTweenSubsystem.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "CoreKitTweenAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitTweenOutput, float, Alpha);

UCLASS(meta = (ExposedAsyncProxy = "Tween"))
class COREKITTWEEN_API UCoreKitTweenAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Plays an eased 0-to-1 value. Use a Lerp node to apply it to any property. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Tween", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DefaultToSelf = "Owner"))
	static UCoreKitTweenAction* PlayTween(const UObject* WorldContextObject, UObject* Owner,
		float Duration, ECoreKitTweenEase Ease = ECoreKitTweenEase::Linear);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Tween", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DefaultToSelf = "Owner"))
	static UCoreKitTweenAction* PlayTweenSequence(const UObject* WorldContextObject, UObject* Owner,
		const TArray<FCoreKitTweenStep>& Steps, const FCoreKitTweenOptions& Options);

	UPROPERTY(BlueprintAssignable)
	FCoreKitTweenOutput Updated;

	UPROPERTY(BlueprintAssignable)
	FCoreKitTweenOutput Completed;

	UPROPERTY(BlueprintAssignable)
	FCoreKitTweenOutput Cancelled;

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Tween")
	void SetPaused(bool bPaused);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Tween")
	void Cancel();

	UFUNCTION(BlueprintPure, Category = "CoreKit|Tween")
	int32 GetCurrentStep() const { return CurrentStep; }

	virtual void Activate() override;

private:
	void Update(int32 StepIndex, float Alpha);
	void Finish(bool bCompleted);

	TWeakObjectPtr<UCoreKitTweenSubsystem> Subsystem;
	TWeakObjectPtr<UObject> TweenOwner;
	uint64 Handle = 0;
	TArray<FCoreKitTweenStep> StepsToPlay;
	FCoreKitTweenOptions TweenOptions;
	int32 CurrentStep = 0;
	float LastAlpha = 0.0f;
	bool bActivated = false;
};
