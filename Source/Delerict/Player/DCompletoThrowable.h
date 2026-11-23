#pragma once

#include "DThrowable.h"
#include "DCompletoThrowable.generated.h"

UCLASS(Blueprintable)
class ADCompletoThrowable : public ADThrowable
{
    GENERATED_BODY()

public:
    ADCompletoThrowable();

    bool CanGrab() const override;
    void AddDriftVelocity(const FVector& VelocityChange, const FVector& PlaneNormal) override;

protected:
    void BeginPlay() override;
    void ResetDeployment() override;

private:
    UFUNCTION()
    void SquashOnImpact(const FHitResult& Hit, const FVector& ImpactVelocity);

    bool bSquashed = false;
};
