#include "LevelDesign/DDockingPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

ADDockingPoint::ADDockingPoint()
{
    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Mount"));
    SetRootComponent(Pivot);

    Fixture = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fixture"));
    Fixture->SetupAttachment(Pivot);
    Fixture->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Fixture->SetGenerateOverlapEvents(false);
    Fixture->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Fixture->SetStaticMesh(Cylinder.Object);
    Fixture->SetRelativeRotation(FRotator(90., 0., 0.));
    Fixture->SetRelativeScale3D(FVector(.45, .45, .16));

    RangePreview = CreateEditorOnlyDefaultSubobject<USphereComponent>(TEXT("ApproachRange"));
    if (RangePreview)
    {
        RangePreview->SetupAttachment(Pivot);
        RangePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RangePreview->SetGenerateOverlapEvents(false);
        RangePreview->SetCanEverAffectNavigation(false);
        RangePreview->SetHiddenInGame(true);
        RangePreview->ShapeColor = FColor::Cyan;
        RangePreview->bDrawOnlyIfSelected = true;

        if (auto* Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("DockingDirection")))
        {
            Arrow->SetupAttachment(RangePreview);
            Arrow->SetHiddenInGame(true);
            Arrow->SetArrowColor(FLinearColor(0.f, 1.f, 1.f));
        }
    }
}

void ADDockingPoint::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (RangePreview)
    {
        RangePreview->SetWorldLocation(GetDockingPose().GetLocation());
        RangePreview->SetWorldScale3D(FVector::OneVector);
        RangePreview->SetSphereRadius(FMath::Max(1.f, ApproachRadius));
    }
}

FTransform ADDockingPoint::GetDockingPose() const
{
    return FTransform(GetActorQuat(), GetActorTransform().TransformPositionNoScale(DockingOffset));
}

bool ADDockingPoint::TryGetDockingPose(const APawn* Pawn, FTransform& OutPose) const
{
    OutPose = FTransform::Identity;
    const FTransform Pose = GetDockingPose();
    if (!bDockingEnabled || !IsValid(Pawn) || Pawn->GetWorld() != GetWorld()
        || FVector::DistSquared(Pawn->GetActorLocation(), Pose.GetLocation()) > FMath::Square(FMath::Max(1.f, ApproachRadius)))
    {
        return false;
    }
    OutPose = Pose;
    return true;
}
