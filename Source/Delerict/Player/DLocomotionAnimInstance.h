#pragma once

#include "Animation/AnimInstance.h"
#include "DLocomotionAnimInstance.generated.h"

UCLASS()
class UDLocomotionAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UDLocomotionAnimInstance();

protected:
    FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
