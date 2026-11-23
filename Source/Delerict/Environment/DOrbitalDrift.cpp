#include "DOrbitalDrift.h"

#include "Components/SceneComponent.h"
#include "CoreKitTween/CoreKitTweenSubsystem.h"
#include "Engine/World.h"

ADOrbitalDrift::ADOrbitalDrift()
{
    USceneComponent* Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
    Pivot->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Pivot);
    SetActorEnableCollision(false);
}

void ADOrbitalDrift::BeginPlay()
{
    Super::BeginPlay();

    InitialRotation = GetActorQuat();
    InitialLocation = GetActorLocation();
    if (UCoreKitTweenSubsystem* Tween = GetWorld()->GetSubsystem<UCoreKitTweenSubsystem>())
    {
        StartRotation(*Tween);
        StartDrift(*Tween);
    }
}

void ADOrbitalDrift::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UCoreKitTweenSubsystem* Tween = GetWorld()->GetSubsystem<UCoreKitTweenSubsystem>())
    {
        Tween->CancelTween(RotationHandle);
        Tween->CancelTween(DriftHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void ADOrbitalDrift::StartRotation(UCoreKitTweenSubsystem& Tween)
{
    const FVector Axis = RotationAxis.GetSafeNormal();
    if (RotationSpeed <= 0.0f || Axis.IsNearlyZero())
    {
        return;
    }

    RotationHandle = Tween.StartTween(this, 360.0f / RotationSpeed, ECoreKitTweenEase::Linear,
        FCoreKitTweenUpdated::CreateWeakLambda(this, [this, Axis](float Alpha)
        {
            SetActorRotation(InitialRotation * FQuat(Axis, 2.0 * UE_DOUBLE_PI * Alpha));
        }),
        FCoreKitTweenFinished::CreateWeakLambda(this, [this, Runner = &Tween](bool bCompleted)
        {
            RotationHandle = 0;
            if (bCompleted)
            {
                StartRotation(*Runner);
            }
        }));
}

void ADOrbitalDrift::StartDrift(UCoreKitTweenSubsystem& Tween)
{
    if (DriftLegSeconds <= 0.0f || DriftOffset.IsNearlyZero())
    {
        return;
    }

    // Return to the authored location at zero speed before repeating the three legs.
    const TArray<FCoreKitTweenStep> Steps = {
        { DriftLegSeconds, ECoreKitTweenEase::SmoothStep },
        { 2.0f * DriftLegSeconds, ECoreKitTweenEase::SmoothStep },
        { DriftLegSeconds, ECoreKitTweenEase::SmoothStep }
    };
    DriftHandle = Tween.StartSequence(this, Steps, {},
        FCoreKitTweenStepUpdated::CreateWeakLambda(this, [this](int32 StepIndex, float Alpha)
        {
            constexpr double Offsets[] = { 0.0, 1.0, -1.0, 0.0 };
            const double Offset = FMath::Lerp(Offsets[StepIndex], Offsets[StepIndex + 1], Alpha);
            SetActorLocation(InitialLocation + DriftOffset * Offset);
        }),
        FCoreKitTweenFinished::CreateWeakLambda(this, [this, Runner = &Tween](bool bCompleted)
        {
            DriftHandle = 0;
            if (bCompleted)
            {
                StartDrift(*Runner);
            }
        }));
}
