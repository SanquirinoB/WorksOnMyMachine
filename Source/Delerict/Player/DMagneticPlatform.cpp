#include "DMagneticPlatform.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DSteelSurfaceComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADMagneticPlatform::ADMagneticPlatform()
{
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitBoxExtent(HalfExtent);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    SteelFace = CreateDefaultSubobject<UDSteelSurfaceComponent>(TEXT("SteelFace"));
    SteelFace->SetupAttachment(Collision);
    SteelFace->SetRelativeLocation(FVector(0., 0., HalfExtent.Z));

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetRelativeScale3D(HalfExtent / 50.);
}

void ADMagneticPlatform::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const FVector Extent = HalfExtent.ComponentMax(FVector(.1));
    Collision->SetBoxExtent(Extent);
    SteelFace->SetRelativeLocation(FVector(0., 0., Extent.Z));
    SteelFace->HalfExtent = FVector2D(Extent.X, Extent.Y);
    Mesh->SetRelativeScale3D(Extent / 50.);
}
