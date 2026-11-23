#include "DLocomotionAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DPawn.h"

struct FDLocomotionProxy : FAnimInstanceProxy
{
    explicit FDLocomotionProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        GaitBlend.A.SetLinkNode(&Walk);
        GaitBlend.B.SetLinkNode(&Run);
        GroundBlend.A.SetLinkNode(&Idle);
        GroundBlend.B.SetLinkNode(&GaitBlend);
        Root.A.SetLinkNode(&GroundBlend);
        Root.B.SetLinkNode(&Airborne);
    }

    FAnimNode_Base* GetCustomRootNode() override { return &Root; }

    void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);

        const ADPawn* Pawn = Cast<ADPawn>(Instance->TryGetPawnOwner());
        if (!Pawn)
        {
            return;
        }

        Idle.SetSequence(Pawn->IdleAnimation);
        Walk.SetSequence(Pawn->WalkAnimation);
        Run.SetSequence(Pawn->RunAnimation);
        Airborne.SetSequence(Pawn->AirborneAnimation);

        const float Speed = Pawn->Mover->GetVelocity().Size2D();
        const float MoveWeight = Pawn->WalkAnimation || Pawn->RunAnimation ? FMath::Clamp(Speed / 120.f, 0.f, 1.f) : 0.f;
        const float RunWeight = Pawn->RunAnimation ? FMath::GetMappedRangeValueClamped(
            FVector2f(160.f, FMath::Max(161.f, Pawn->MaxGroundSpeed)), FVector2f(0.f, 1.f), Speed) : 0.f;
        GroundBlend.Alpha = FMath::FInterpTo(GroundBlend.Alpha, MoveWeight, DeltaSeconds, 12.f);
        GaitBlend.Alpha = FMath::FInterpTo(GaitBlend.Alpha, RunWeight, DeltaSeconds, 12.f);
        const float AirWeight = Pawn->AirborneAnimation && Pawn->Mover->IsAirborne() ? 1.f : 0.f;
        Root.Alpha = FMath::FInterpTo(Root.Alpha, AirWeight, DeltaSeconds, 14.f);
    }

    FAnimNode_SequencePlayer_Standalone Idle;
    FAnimNode_SequencePlayer_Standalone Walk;
    FAnimNode_SequencePlayer_Standalone Run;
    FAnimNode_SequencePlayer_Standalone Airborne;
    FAnimNode_TwoWayBlend GaitBlend;
    FAnimNode_TwoWayBlend GroundBlend;
    FAnimNode_TwoWayBlend Root;
};

UDLocomotionAnimInstance::UDLocomotionAnimInstance()
{
    RootMotionMode = ERootMotionMode::IgnoreRootMotion;
}

FAnimInstanceProxy* UDLocomotionAnimInstance::CreateAnimInstanceProxy()
{
    return new FDLocomotionProxy(this);
}

void UDLocomotionAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)
{
    delete Proxy;
}
