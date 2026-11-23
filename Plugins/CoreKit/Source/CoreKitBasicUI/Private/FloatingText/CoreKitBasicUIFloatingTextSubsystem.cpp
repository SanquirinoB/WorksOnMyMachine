#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextSubsystem.h"
#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextTypes.h"
#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UCoreKitBasicUIFloatingTextSubsystem::Initialize(FSubsystemCollectionBase& C)
{
    Super::Initialize(C);Random.Initialize(int32(FPlatformTime::Cycles()));
    CleanupHandle=FWorldDelegates::OnWorldCleanup.AddUObject(this,&ThisClass::WorldCleanup);
}
void UCoreKitBasicUIFloatingTextSubsystem::Deinitialize()
{ FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);Clear();Definition=nullptr;Super::Deinitialize(); }
void UCoreKitBasicUIFloatingTextSubsystem::WorldCleanup(UWorld* World,bool,bool)
{ if (ActiveWorld.Get()==World) { Clear();Definition=nullptr; } }
void UCoreKitBasicUIFloatingTextSubsystem::Clear()
{
    for (const auto& Item:Pool) if (Item) Item->RemoveFromParent();
    Active.Reset();Pool.Reset();if (Layer) Layer->RemoveFromParent();Layer=nullptr;ActiveWorld.Reset();
}
bool UCoreKitBasicUIFloatingTextSubsystem::EnsureLayer()
{
    UWorld* World=GetWorld();auto* Player=GetLocalPlayer();
    if (!World || !Player || !Player->ViewportClient || World->GetNetMode()==NM_DedicatedServer || !Player->GetPlayerController(World)) return false;
    if (Layer && ActiveWorld.Get()!=World) Clear();
    if (!Layer)
    {
        Layer=CreateWidget<UCoreKitBasicUIFloatingTextLayerWidget>(Player->GetPlayerController(World),UCoreKitBasicUIFloatingTextLayerWidget::StaticClass());
        if (!Layer || !Layer->AddToPlayerScreen(25)) { Layer=nullptr;return false; }
        ActiveWorld=World;
        if (Definition && Pool.IsEmpty())
            for (int I=0;I<Definition->InitialPoolSize;++I)
                if (auto* Item=CreateWidget<UCoreKitBasicUIFloatingTextWidget>(Player->GetPlayerController(World),UCoreKitBasicUIFloatingTextWidget::StaticClass())) Pool.Add(Item);
    }
    return true;
}
bool UCoreKitBasicUIFloatingTextSubsystem::Configure(UCoreKitBasicUIFloatingTextDefinition* Config)
{
    Clear();Definition=nullptr;Submitted=Reused=Evicted=0;LastStyle=NAME_None;LastValue=FText::GetEmpty();
    if (!Config || !Config->IsUsable()) return false;Definition=Config;
    // A configuration is valid before a viewport exists; allocation is deferred until needed.
    EnsureLayer();
    return true;
}
bool UCoreKitBasicUIFloatingTextSubsystem::ShowText(const FText& V,FName S,FVector P,AActor* A) { return Submit(V,S,P,A,false,FVector2D::ZeroVector); }
bool UCoreKitBasicUIFloatingTextSubsystem::ShowScreenText(const FText& V,FName S,FVector2D P) { return Submit(V,S,FVector::ZeroVector,nullptr,true,P); }
bool UCoreKitBasicUIFloatingTextSubsystem::Submit(const FText& Value,FName Style,FVector Position,AActor* Actor,bool bScreen,FVector2D Screen)
{
    const auto* Options=Definition?Definition->FindStyle(Style):nullptr;
    if (!Options || !Options->IsUsable() || Position.ContainsNaN() || Screen.ContainsNaN() || !EnsureLayer()) return false;
    if (Actor && Actor->GetWorld()!=GetWorld()) return false;
    if (Active.Num()>=Definition->MaxTexts) { Release(0);++Evicted; }
    int32 Index=INDEX_NONE;
    for (int32 I=0;I<Pool.Num();++I)
        if (!Active.ContainsByPredicate([I](const auto& E){ return E.PoolIndex==I; })) { Index=I;++Reused;break; }
    if (Index==INDEX_NONE)
    {
        auto* Item=CreateWidget<UCoreKitBasicUIFloatingTextWidget>(GetLocalPlayer()->GetPlayerController(GetWorld()),UCoreKitBasicUIFloatingTextWidget::StaticClass());if (!Item) return false;
        Index=Pool.Add(Item);
    }
    auto* Item=Pool[Index].Get();Layer->AddText(Item);Item->Start(Value,*Options,Random);
    FCoreKitBasicUIFloatingTextEntry E;E.PoolIndex=Index;E.WorldPosition=Position;E.Actor=Actor;E.Group=FObjectKey(Actor);
    E.ActorOffset=Actor?Position-Actor->GetActorLocation():FVector::ZeroVector;E.bScreenAnchor=bScreen;E.ScreenPosition=Screen;
    E.Lifetime=Options->GetLifetime();E.Serial=++Serial;
    if (Actor) for (auto& Old:Active) if (Old.Group==E.Group) Old.LayerOpacity=FMath::Max(0.f,Old.LayerOpacity-Definition->OpacityDecreasing);
    Active.Add(E);++Submitted;LastStyle=Style;LastValue=Value;Item->ForceLayoutPrepass();
    Tick(0.f);return true;
}
void UCoreKitBasicUIFloatingTextSubsystem::Release(int32 Index)
{ if (Active.IsValidIndex(Index)) { Pool[Active[Index].PoolIndex]->RemoveFromParent();Active.RemoveAt(Index); } }
bool UCoreKitBasicUIFloatingTextSubsystem::IsTickable() const
{ return !IsTemplate() && Definition && Layer && !Active.IsEmpty() && GetWorld() && !UGameplayStatics::IsGamePaused(GetWorld()); }
int32 UCoreKitBasicUIFloatingTextSubsystem::GetVisibleCount() const
{ int32 Count=0;for (const auto& E:Active) if (Pool[E.PoolIndex]->IsVisible()) ++Count;return Count; }
void UCoreKitBasicUIFloatingTextSubsystem::Tick(float Delta)
{
    auto* World=GetWorld();auto* PC=GetLocalPlayer()?GetLocalPlayer()->GetPlayerController(World):nullptr;
    if (!World || !PC || !Layer || !Definition || UGameplayStatics::IsGamePaused(World)) return;
    // World delta respects full pause while remaining independent of a victim's hitstop dilation.
    Delta=Delta>0.f?World->GetDeltaSeconds():0.f;
    for (int I=Active.Num()-1;I>=0;--I)
    { Active[I].Age+=FMath::Max(0.f,Delta);if (Active[I].Age>=Active[I].Lifetime) Release(I); }
    FVector Eye;FRotator Rotation;PC->GetPlayerViewPoint(Eye,Rotation);
    const float ViewScale=FMath::Max(.01f,UWidgetLayoutLibrary::GetViewportScale(this));
    const FVector2D Size=UWidgetLayoutLibrary::GetViewportSize(this)/ViewScale;
    for (int I=0;I<Active.Num();++I)
    {
        auto& E=Active[I];auto* Item=Pool[E.PoolIndex].Get();FVector2D P=E.ScreenPosition;float Scale=1.f;
        if (!E.bScreenAnchor)
        {
            if (Definition->bFollowTarget && E.Actor.IsValid()) E.WorldPosition=E.Actor->GetActorLocation()+E.ActorOffset;
            if (FVector::DotProduct(E.WorldPosition-Eye,Rotation.Vector())<=0.f || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC,E.WorldPosition,P,true))
            { Item->SetVisibility(ESlateVisibility::Hidden);continue; }
            P+=Definition->ScreenOffset;Scale=Definition->DistanceScale(FVector::Distance(Eye,E.WorldPosition));
        }
        if (P.X<0 || P.Y<0 || P.X>Size.X || P.Y>Size.Y) { Item->SetVisibility(ESlateVisibility::Hidden);continue; }
        Item->SetVisibility(ESlateVisibility::HitTestInvisible);
        Item->Animate(E.Age,E.LayerOpacity,Scale);
        // Clamp the animated glyph/icon bounds, not only the projected anchor; preserve DPI units.
        const FVector2D Half=Item->GetDesiredSize()*Item->GetRenderTransform().Scale*.5f;
        const FVector2D Translation=Item->GetRenderTransform().Translation;
        const FVector2D Margin(FMath::Min(Half.X+8.f,Size.X*.5f),FMath::Min(Half.Y+8.f,Size.Y*.5f));
        const FVector2D Final=P+Translation;
        P=FVector2D(FMath::Clamp(Final.X,Margin.X,Size.X-Margin.X),FMath::Clamp(Final.Y,Margin.Y,Size.Y-Margin.Y))-Translation;
        if (auto* CanvasSlot=Cast<UCanvasPanelSlot>(Item->Slot)) CanvasSlot->SetPosition(P);
    }
}
