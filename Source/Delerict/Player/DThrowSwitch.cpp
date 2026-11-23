#include "DThrowSwitch.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADThrowSwitch::ADThrowSwitch()
{
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitBoxExtent(FVector(10., 35., 35.));
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetGenerateOverlapEvents(false);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Collision);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeScale3D(FVector(.20, .70, .70));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Cube.Object);

    Indicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActiveIndicator"));
    Indicator->SetupAttachment(Collision);
    Indicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Indicator->SetRelativeLocation(FVector(0., 37., 0.));
    Indicator->SetRelativeScale3D(FVector(.09, .08, .29));
    Indicator->SetStaticMesh(Cube.Object);
    Indicator->SetVisibility(false);
}

void ADThrowSwitch::Activate()
{
    if (!bActivated)
    {
        bActivated = true;
        Indicator->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.1, 1., 0.25));
        Indicator->SetVisibility(true);
        OnActivated.Broadcast();
    }
}
