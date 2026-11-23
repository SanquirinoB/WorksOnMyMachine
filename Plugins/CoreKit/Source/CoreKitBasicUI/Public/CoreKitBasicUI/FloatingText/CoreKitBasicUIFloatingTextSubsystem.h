#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "UObject/ObjectKey.h"
#include "CoreKitBasicUIFloatingTextSubsystem.generated.h"
class UCoreKitBasicUIFloatingTextDefinition;
class UCoreKitBasicUIFloatingTextWidget;
class UCoreKitBasicUIFloatingTextLayerWidget;

struct FCoreKitBasicUIFloatingTextEntry
{
    int32 PoolIndex=INDEX_NONE;
    TWeakObjectPtr<AActor> Actor;
    FObjectKey Group;
    FVector WorldPosition=FVector::ZeroVector,ActorOffset=FVector::ZeroVector;
    FVector2D ScreenPosition=FVector2D::ZeroVector;
    float Age=0.f,Lifetime=0.f,LayerOpacity=1.f;
    bool bScreenAnchor=false;
    uint64 Serial=0;
};

/** Cosmetic, local-only feedback. Never changes damage, health, input or network authority. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIFloatingTextSubsystem:public ULocalPlayerSubsystem,public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable,Category="CoreKit|BasicUI|FloatingText") bool Configure(UCoreKitBasicUIFloatingTextDefinition* Config);
    UFUNCTION(BlueprintCallable,Category="CoreKit|BasicUI|FloatingText") bool ShowText(const FText& Value,FName Style,FVector WorldPosition,AActor* FollowActor=nullptr);
    /** Screen positions are logical UMG coordinates; intended for explicit notices/style previews. */
    UFUNCTION(BlueprintCallable,Category="CoreKit|BasicUI|FloatingText") bool ShowScreenText(const FText& Value,FName Style,FVector2D ScreenPosition);
    UFUNCTION(BlueprintCallable,Category="CoreKit|BasicUI|FloatingText") void Clear();
    UFUNCTION(BlueprintPure,Category="CoreKit|BasicUI|FloatingText") int32 GetActiveCount() const { return Active.Num(); }
    UFUNCTION(BlueprintPure,Category="CoreKit|BasicUI|FloatingText") int32 GetAllocatedCount() const { return Pool.Num(); }
    UCoreKitBasicUIFloatingTextDefinition* GetDefinition() const { return Definition; }
    int32 GetSubmittedCount() const { return Submitted; }
    int32 GetReusedCount() const { return Reused; }
    int32 GetEvictedCount() const { return Evicted; }
    FName GetLastStyle() const { return LastStyle; }
    FText GetLastValue() const { return LastValue; }
    float GetOldestAge() const { return Active.IsEmpty()?0.f:Active[0].Age; }
    int32 GetVisibleCount() const;
    FVector GetNewestWorldPosition() const { return Active.IsEmpty()?FVector::ZeroVector:Active.Last().WorldPosition; }
    virtual void Tick(float DeltaSeconds) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(CoreKitBasicUIFloatingText,STATGROUP_Tickables); }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
private:
    UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIFloatingTextDefinition> Definition;
    UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIFloatingTextLayerWidget> Layer;
    UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIFloatingTextWidget>> Pool;
    TArray<FCoreKitBasicUIFloatingTextEntry> Active;
    TWeakObjectPtr<UWorld> ActiveWorld;
    FDelegateHandle CleanupHandle;
    FRandomStream Random;
    uint64 Serial=0;
    int32 Submitted=0,Reused=0,Evicted=0;
    FName LastStyle;
    FText LastValue;
    bool EnsureLayer();
    bool Submit(const FText& Value,FName Style,FVector Position,AActor* Actor,bool bScreen,FVector2D Screen);
    void Release(int32 Index);
    void WorldCleanup(UWorld* World,bool bSessionEnded,bool bCleanupResources);
};
