#pragma once

#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUIEvents.generated.h"

/** Transient UI events (Docs/UI/Spec/03-HUDEvents.md). Payload structs below; tags under UI.Event. */
namespace CoreKitBasicUIEvent
{
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hit);              // FCoreKitBasicUIHitEvent (also kills)
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(DamageTaken);      // FCoreKitBasicUIDamageTakenEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Healed);           // FCoreKitBasicUIPickupEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pickup);           // FCoreKitBasicUIPickupEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dashed);           // none
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(DashReady);        // none
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(WeaponSwapped);    // FCoreKitBasicUIWeaponEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReloadStarted);    // FCoreKitBasicUIWeaponEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReloadCompleted);  // FCoreKitBasicUIWeaponEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(LevelUp);          // FCoreKitBasicUILevelEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardChosen);       // FCoreKitBasicUICardEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(StageChanged);     // FCoreKitBasicUIStageEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(BossInbound);      // none
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(BossSpawned);      // none
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(BossTelegraph);    // FCoreKitBasicUIBossAttackEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(BossDefeated);     // none
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ThreatLock);       // FCoreKitBasicUIThreatEvent
	COREKITBASICUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(RunEnded);         // FCoreKitBasicUIRunEndEvent
}

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIHitEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") ECoreKitBasicUIDamageSource Source = ECoreKitBasicUIDamageSource::Rifle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float Amount = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FVector WorldLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bKill = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bPierce = false;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIDamageTakenEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float Amount = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bHasSource = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FVector SourceLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIPickupEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") ECoreKitBasicUIPickupKind Kind = ECoreKitBasicUIPickupKind::XP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float Amount = 0.f;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIWeaponEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 SlotNumber = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText LongName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bMelee = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float Duration = 0.f;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUILevelEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 Level = 1;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUICardEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FCoreKitBasicUICardData Card;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bStacked = false;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIStageEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 StageIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText Subtitle;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIBossAttackEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText AttackName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FText Hint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float Duration = 0.f;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIThreatEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") TWeakObjectPtr<AActor> Source;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bLocking = false;
};

USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIRunEndEvent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bVictory = false;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FCoreKitBasicUIEventDelegate, FGameplayTag /*Event*/, const FInstancedStruct& /*Payload*/);

/** Routes transient UI events by tag. Pure routing: no state. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIEventChannel : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void Publish(FGameplayTag Event, const FInstancedStruct& Payload);

	template <typename T>
	void Publish(FGameplayTag Event, const T& Payload) { Publish(Event, FInstancedStruct::Make(Payload)); }

	/** Subscribe to one tag (exact match). */
	FDelegateHandle Subscribe(FGameplayTag Event, FCoreKitBasicUIEventDelegate::FDelegate Delegate);
	void Unsubscribe(FGameplayTag Event, FDelegateHandle Handle);

	/** Every event (for widgets that react to several). */
	FCoreKitBasicUIEventDelegate OnAnyEvent;

private:
	TMap<FGameplayTag, FCoreKitBasicUIEventDelegate> Listeners;
};
