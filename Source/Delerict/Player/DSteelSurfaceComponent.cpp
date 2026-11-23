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

FVector UDSteelSurfaceComponent::LimitTravel(const FVector& Point, const FVector& Delta) const
{
    const FTransform& Transform = GetComponentTransform();
    const FVector LocalPoint = Transform.InverseTransformPosition(Point);
    const FVector LocalDelta = Transform.InverseTransformVector(Delta);
    const auto EdgeFraction = [](double Position, double Direction, double Extent)
    {
        if (FMath::IsNearlyZero(Direction, UE_DOUBLE_SMALL_NUMBER))
        {
            return 1.;
        }
        const double Boundary = Direction > 0. ? Extent : -Extent;
        return (Boundary - Position) / Direction;
    };
    const double Fraction = FMath::Min(EdgeFraction(LocalPoint.X, LocalDelta.X, HalfExtent.X),
        EdgeFraction(LocalPoint.Y, LocalDelta.Y, HalfExtent.Y));
    return Delta * FMath::Clamp(Fraction, 0., 1.);
}
