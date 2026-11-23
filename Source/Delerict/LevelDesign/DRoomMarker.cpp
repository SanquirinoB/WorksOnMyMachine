#include "LevelDesign/DRoomMarker.h"

#include "DRoomProgress.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADRoomMarker::ADRoomMarker()
{
    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("BottomPivot"));
    SetRootComponent(Pivot);

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    Trigger->SetupAttachment(Pivot);
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionObjectType(ECC_WorldDynamic);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->SetGenerateOverlapEvents(true);
    Trigger->SetCanEverAffectNavigation(false);
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnEntered);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Visual->SetupAttachment(Pivot);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetGenerateOverlapEvents(false);
    Visual->SetCanEverAffectNavigation(false);
    Visual->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Visual->SetStaticMesh(Cube.Object);

    Caption = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Caption"));
    Caption->SetupAttachment(Pivot);
    Caption->SetRelativeRotation(FRotator(0, 90, 0));
    Caption->SetHorizontalAlignment(EHTA_Center);
    Caption->SetWorldSize(26.f);
}

void ADRoomMarker::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const FVector Size = Dimensions.ComponentMax(FVector::OneVector);
    Trigger->SetBoxExtent(Size * .5);
    Trigger->SetRelativeLocation(FVector(0, 0, Size.Z * .5));

    const bool bHazard = MarkerRole == EDRoomMarkerRole::Hazard;
    const FVector VisualSize = bHazard ? Size : FVector(14, 10, FMath::Min(Size.Z, 130.));
    const double DisplayDepth = bHazard ? Size.Y : -Size.Y * .35;
    Visual->SetRelativeLocation(FVector(0, bHazard ? 0 : DisplayDepth, VisualSize.Z * .5));
    Visual->SetRelativeScale3D(VisualSize / 100.);
    Visual->SetMaterial(0, Finish);
    Caption->SetRelativeLocation(FVector(0, bHazard ? Size.Y * .5 + 4. : DisplayDepth + 12., VisualSize.Z + 30.));
    Caption->SetText(Label);
}

void ADRoomMarker::OnEntered(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
    int32, bool, const FHitResult&)
{
    APawn* Pawn = Cast<APawn>(OtherActor);
    APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!Player || !HasAuthority() || OtherComponent != Pawn->GetRootComponent())
    {
        return;
    }
    UDRoomProgress* Progress = GetWorld()->GetSubsystem<UDRoomProgress>();
    if (!Progress)
    {
        return;
    }

    switch (MarkerRole)
    {
    case EDRoomMarkerRole::Hazard:
        Progress->RequestRespawn(*Player, *Pawn);
        return;
    case EDRoomMarkerRole::Checkpoint:
        if (!Progress->SetCheckpoint(*Player, FTransform(GetActorRotation(),
            GetActorTransform().TransformPositionNoScale(RespawnOffset))))
        {
            return;
        }
        break;
    case EDRoomMarkerRole::Exit:
        if (!Progress->CompleteRoom(*Player))
        {
            return;
        }
        break;
    }

    if (GEngine && !Feedback.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::White, Feedback.ToString());
    }
    OnMarkerActivated(Pawn);
    if (MarkerRole == EDRoomMarkerRole::Exit && !Destination.IsNull())
    {
        UGameplayStatics::OpenLevelBySoftObjectPtr(this, Destination);
    }
}
