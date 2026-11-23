#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "CoreKitTweenSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECoreKitTweenEase : uint8
{
	Linear,
	SmoothStep,
	QuadraticIn,
	QuadraticOut,
	QuadraticInOut
};

UENUM(BlueprintType)
enum class ECoreKitTweenClock : uint8
{
	Game,
	RealTime
};

UENUM(BlueprintType)
enum class ECoreKitTweenConflict : uint8
{
	Allow,
	Reject,
	Replace
};

USTRUCT(BlueprintType)
struct COREKITTWEEN_API FCoreKitTweenOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Tween")
	ECoreKitTweenClock Clock = ECoreKitTweenClock::Game;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Tween")
	FName Channel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Tween")
	ECoreKitTweenConflict Conflict = ECoreKitTweenConflict::Allow;
};

USTRUCT(BlueprintType)
struct COREKITTWEEN_API FCoreKitTweenStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Tween", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Tween")
	ECoreKitTweenEase Ease = ECoreKitTweenEase::Linear;
};

DECLARE_DELEGATE_OneParam(FCoreKitTweenUpdated, float);
DECLARE_DELEGATE_TwoParams(FCoreKitTweenStepUpdated, int32, float);
DECLARE_DELEGATE_OneParam(FCoreKitTweenFinished, bool);

UCLASS()
class COREKITTWEEN_API UCoreKitTweenSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Returns a world-local handle. Updates and completion run on the game thread. */
	uint64 StartTween(UObject* Owner, float Duration, ECoreKitTweenEase Ease,
		FCoreKitTweenUpdated OnUpdate, FCoreKitTweenFinished OnFinish = {});
	uint64 StartSequence(UObject* Owner, const TArray<FCoreKitTweenStep>& Steps, const FCoreKitTweenOptions& Options,
		FCoreKitTweenStepUpdated OnUpdate, FCoreKitTweenFinished OnFinish = {});
	bool SetTweenPaused(uint64 Handle, bool bPaused);
	bool CancelTween(uint64 Handle);

	UFUNCTION(BlueprintCallable, Category = "CoreKit|Tween")
	int32 CancelTweensByChannel(UObject* Owner, FName Channel);

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !Tweens.IsEmpty(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	struct FTween
	{
		TWeakObjectPtr<UObject> Owner;
		TArray<FCoreKitTweenStep> Steps;
		FCoreKitTweenOptions Options;
		int32 StepIndex = 0;
		double Elapsed = 0.0;
		double LastRealTime = 0.0;
		bool bPaused = false;
		FCoreKitTweenUpdated OnUpdate;
		FCoreKitTweenStepUpdated OnStepUpdated;
		FCoreKitTweenFinished OnFinish;
	};

	bool FinishTween(uint64 Handle, bool bCompleted);
	uint64 AddTween(FTween&& Tween);
	TArray<uint64> FindChannelTweens(UObject* Owner, FName Channel) const;
	void AdvanceTween(uint64 Handle, double Budget);

	TMap<uint64, FTween> Tweens;
	uint64 LastHandle = 0;
};
