#include "LevelDesign/DLevelBlock.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADLevelBlock::ADLevelBlock()
{
    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("BottomPivot"));
    SetRootComponent(Pivot);
    Pivot->SetMobility(EComponentMobility::Static);

    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    Collision->SetupAttachment(Pivot);
    Collision->SetMobility(EComponentMobility::Static);
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    Collision->SetGenerateOverlapEvents(false);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Pivot);
    Visual->SetMobility(EComponentMobility::Static);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetGenerateOverlapEvents(false);
    Visual->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Visual->SetStaticMesh(Cube.Object);
}

void ADLevelBlock::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const FVector Size = Dimensions.ComponentMax(FVector::OneVector);
    const FVector Center(0.0, 0.0, Size.Z * .5);
    Collision->SetBoxExtent(Size * .5);
    Collision->SetRelativeLocation(Center);
    Visual->SetRelativeLocation(Center);
    Visual->SetRelativeScale3D(Size / 100.0);
    Visual->SetMaterial(0, Finish);
}
