#include "DSteelSurfaceComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"

bool UDSteelSurfaceComponent::CanSupport(const UCapsuleComponent& Capsule) const
{
    const auto* Collider = Cast<UPrimitiveComponent>(GetAttachParent());
    return bMagnetic && IsRegistered() && Collider && Collider->IsRegistered() && Collider->IsQueryCollisionEnabled()
        && Collider->GetCollisionResponseToChannel(Capsule.GetCollisionObjectType()) == ECR_Block
        && Capsule.GetCollisionResponseToChannel(Collider->GetCollisionObjectType()) == ECR_Block;
}

bool UDSteelSurfaceComponent::ContainsProjection(const FVector& Point, double Tolerance) const
{
    const FVector LocalPoint = GetComponentTransform().InverseTransformPosition(Point);
    return FMath::Abs(LocalPoint.X) <= HalfExtent.X + Tolerance
        && FMath::Abs(LocalPoint.Y) <= HalfExtent.Y + Tolerance;
}

FVector UDSteelSurfaceComponent::ClosestPoint(const FVector& Point) const
{
    const FTransform& Transform = GetComponentTransform();
    const FVector LocalPoint = Transform.InverseTransformPosition(Point);
    return Transform.TransformPosition(FVector(
        FMath::Clamp(LocalPoint.X, -HalfExtent.X, HalfExtent.X),
        FMath::Clamp(LocalPoint.Y, -HalfExtent.Y, HalfExtent.Y),
        0.));
}
