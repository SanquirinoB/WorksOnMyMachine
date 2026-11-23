#pragma once

#include "MVVMViewModelBase.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUIViewModels.generated.h"

DECLARE_DELEGATE(FCoreKitBasicUIRunCommand);

/*
 * Per-domain ViewModels (UE MVVM, FieldNotify). Only data: a Presenter writes its ViewModel; widgets subscribe in C++
 * with AddFieldValueChangedDelegate (or read on tick for continuous values). See Docs/UI/Spec/10-Data.md.
 */

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIRunViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float ElapsedSeconds = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetElapsedSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ElapsedSeconds, InValue); }
	const float& GetElapsedSeconds() const { return ElapsedSeconds; }

	/** Seconds until the configured boss arrival, clamped at zero. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float RemainingSeconds = 240.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetRemainingSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(RemainingSeconds, InValue); }
	const float& GetRemainingSeconds() const { return RemainingSeconds; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float SurvivalSeconds = 240.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetSurvivalSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(SurvivalSeconds, InValue); }
	const float& GetSurvivalSeconds() const { return SurvivalSeconds; }

	/** 0..3 = INTRO, PRESSURE, SURVIVAL, CHAOS. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 StageIndex = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetStageIndex(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(StageIndex, InValue); }
	const int32& GetStageIndex() const { return StageIndex; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText StageName;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetStageName(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(StageName, InValue); }
	const FText& GetStageName() const { return StageName; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText StageSubtitle;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetStageSubtitle(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(StageSubtitle, InValue); }
	const FText& GetStageSubtitle() const { return StageSubtitle; }

	/** Fraction of the current stage. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float StageProgress = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetStageProgress(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(StageProgress, InValue); }
	const float& GetStageProgress() const { return StageProgress; }

	/** Stage boundaries in seconds (start of each stage, plus the end). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<float> StageStarts;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetStageStarts(const TArray<float>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(StageStarts, InValue); }
	const TArray<float>& GetStageStarts() const { return StageStarts; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	ECoreKitBasicUIRunPhase Phase = ECoreKitBasicUIRunPhase::Survival;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetPhase(const ECoreKitBasicUIRunPhase& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Phase, InValue); }
	const ECoreKitBasicUIRunPhase& GetPhase() const { return Phase; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bPaused = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbPaused(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bPaused, InValue); }
	const bool& GetbPaused() const { return bPaused; }

	/** Seconds left on the 09:50 boss warning; negative when not counting. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float BossInboundSeconds = -1.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetBossInboundSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(BossInboundSeconds, InValue); }
	const float& GetBossInboundSeconds() const { return BossInboundSeconds; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float BossOvertimeSeconds = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetBossOvertimeSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(BossOvertimeSeconds, InValue); }
	const float& GetBossOvertimeSeconds() const { return BossOvertimeSeconds; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 AliveDrones = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAliveDrones(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(AliveDrones, InValue); }
	const int32& GetAliveDrones() const { return AliveDrones; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Kills = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetKills(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Kills, InValue); }
	const int32& GetKills() const { return Kills; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText ArenaName;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetArenaName(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ArenaName, InValue); }
	const FText& GetArenaName() const { return ArenaName; }


	/** Commands executed by the run presenter (UI never touches gameplay directly). */
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void RequestTogglePause() { TogglePause.ExecuteIfBound(); }
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void RequestRestart() { Restart.ExecuteIfBound(); }
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void RequestQuitToMenu() { QuitToMenu.ExecuteIfBound(); }
	FCoreKitBasicUIRunCommand TogglePause;
	FCoreKitBasicUIRunCommand Restart;
	FCoreKitBasicUIRunCommand QuitToMenu;
};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIVitalsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float Health = 100.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetHealth(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Health, InValue); }
	const float& GetHealth() const { return Health; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float MaxHealth = 100.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetMaxHealth(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, InValue); }
	const float& GetMaxHealth() const { return MaxHealth; }

	/** Below 30 %. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bLowHealth = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbLowHealth(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bLowHealth, InValue); }
	const bool& GetbLowHealth() const { return bLowHealth; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bDashReady = true;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbDashReady(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bDashReady, InValue); }
	const bool& GetbDashReady() const { return bDashReady; }

	/** 0 = just used, 1 = ready. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float DashCharge = 1.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetDashCharge(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(DashCharge, InValue); }
	const float& GetDashCharge() const { return DashCharge; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float DashCooldownRemaining = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetDashCooldownRemaining(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(DashCooldownRemaining, InValue); }
	const float& GetDashCooldownRemaining() const { return DashCooldownRemaining; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Dashes = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetDashes(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Dashes, InValue); }
	const int32& GetDashes() const { return Dashes; }

};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUILoadoutViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<FCoreKitBasicUIWeaponSlotData> Slots;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetSlots(const TArray<FCoreKitBasicUIWeaponSlotData>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Slots, InValue); }
	const TArray<FCoreKitBasicUIWeaponSlotData>& GetSlots() const { return Slots; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 ActiveSlot = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetActiveSlot(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ActiveSlot, InValue); }
	const int32& GetActiveSlot() const { return ActiveSlot; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bMeleeActive = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbMeleeActive(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bMeleeActive, InValue); }
	const bool& GetbMeleeActive() const { return bMeleeActive; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Ammo = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAmmo(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Ammo, InValue); }
	const int32& GetAmmo() const { return Ammo; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 MagazineSize = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetMagazineSize(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(MagazineSize, InValue); }
	const int32& GetMagazineSize() const { return MagazineSize; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bInfiniteReserve = true;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbInfiniteReserve(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bInfiniteReserve, InValue); }
	const bool& GetbInfiniteReserve() const { return bInfiniteReserve; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bReloading = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbReloading(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bReloading, InValue); }
	const bool& GetbReloading() const { return bReloading; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float ReloadProgress = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetReloadProgress(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ReloadProgress, InValue); }
	const float& GetReloadProgress() const { return ReloadProgress; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float ReloadDuration = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetReloadDuration(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ReloadDuration, InValue); }
	const float& GetReloadDuration() const { return ReloadDuration; }

	/** Below 25 % of the magazine. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bLowAmmo = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbLowAmmo(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bLowAmmo, InValue); }
	const bool& GetbLowAmmo() const { return bLowAmmo; }

	/** Crosshair colour from settings. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FLinearColor CrosshairColor = FLinearColor::White;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetCrosshairColor(const FLinearColor& InValue) { UE_MVVM_SET_PROPERTY_VALUE(CrosshairColor, InValue); }
	const FLinearColor& GetCrosshairColor() const { return CrosshairColor; }

};

DECLARE_DELEGATE_RetVal_OneParam(bool, FCoreKitBasicUIChooseCardCommand, int32 /*OfferIndex*/);

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIProgressionViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Level = 1;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetLevel(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Level, InValue); }
	const int32& GetLevel() const { return Level; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float XPFraction = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetXPFraction(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(XPFraction, InValue); }
	const float& GetXPFraction() const { return XPFraction; }

	/** Level ups still waiting after the current offer. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 QueuedDraws = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetQueuedDraws(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(QueuedDraws, InValue); }
	const int32& GetQueuedDraws() const { return QueuedDraws; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bOfferOpen = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbOfferOpen(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bOfferOpen, InValue); }
	const bool& GetbOfferOpen() const { return bOfferOpen; }

	/** Changes whenever a new offer opens. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 OfferSerial = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetOfferSerial(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(OfferSerial, InValue); }
	const int32& GetOfferSerial() const { return OfferSerial; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<FCoreKitBasicUICardData> Offer;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetOffer(const TArray<FCoreKitBasicUICardData>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Offer, InValue); }
	const TArray<FCoreKitBasicUICardData>& GetOffer() const { return Offer; }

public:
	/** Command: choose a card of the current offer. Executed by the presenter. */
	bool RequestChooseCard(int32 OfferIndex) { return ChooseCard.IsBound() && ChooseCard.Execute(OfferIndex); }
	FCoreKitBasicUIChooseCardCommand ChooseCard;
};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIBuildViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Distinct cards in pick order, each with Copies and SummedEffect filled. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<FCoreKitBasicUICardData> Cards;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetCards(const TArray<FCoreKitBasicUICardData>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Cards, InValue); }
	const TArray<FCoreKitBasicUICardData>& GetCards() const { return Cards; }

	/** Total copies ("BUILD · n CARDS"). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 TotalCopies = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTotalCopies(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(TotalCopies, InValue); }
	const int32& GetTotalCopies() const { return TotalCopies; }

	/** "RIFLE-HEAVY", "BLADE-HEAVY", "DASH-HEAVY", "ARMOR-HEAVY". */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText Archetype;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetArchetype(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Archetype, InValue); }
	const FText& GetArchetype() const { return Archetype; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FName NewestCard = NAME_None;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetNewestCard(const FName& InValue) { UE_MVVM_SET_PROPERTY_VALUE(NewestCard, InValue); }
	const FName& GetNewestCard() const { return NewestCard; }

	/** Increments on every pick (new card or stack). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 PickSerial = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetPickSerial(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(PickSerial, InValue); }
	const int32& GetPickSerial() const { return PickSerial; }


	/** True while the player holds Inspect build (TAB). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bInspecting = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbInspecting(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bInspecting, InValue); }
	const bool& GetbInspecting() const { return bInspecting; }
};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIBossViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bVisible = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbVisible(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bVisible, InValue); }
	const bool& GetbVisible() const { return bVisible; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText Name;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetName(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Name, InValue); }
	const FText& GetName() const { return Name; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText Subtitle;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetSubtitle(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Subtitle, InValue); }
	const FText& GetSubtitle() const { return Subtitle; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float Health = 1.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetHealth(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Health, InValue); }
	const float& GetHealth() const { return Health; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bAttackActive = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbAttackActive(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bAttackActive, InValue); }
	const bool& GetbAttackActive() const { return bAttackActive; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText AttackName;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAttackName(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(AttackName, InValue); }
	const FText& GetAttackName() const { return AttackName; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText AttackHint;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAttackHint(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(AttackHint, InValue); }
	const FText& GetAttackHint() const { return AttackHint; }

	/** 0..1 telegraph progress of the current attack. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float TelegraphProgress = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTelegraphProgress(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(TelegraphProgress, InValue); }
	const float& GetTelegraphProgress() const { return TelegraphProgress; }

	/** Area of the current attack in world XY (radius 0 = none). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FVector2D AreaCenter = FVector2D::ZeroVector;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAreaCenter(const FVector2D& InValue) { UE_MVVM_SET_PROPERTY_VALUE(AreaCenter, InValue); }
	const FVector2D& GetAreaCenter() const { return AreaCenter; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float AreaRadius = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAreaRadius(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(AreaRadius, InValue); }
	const float& GetAreaRadius() const { return AreaRadius; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FVector2D Position = FVector2D::ZeroVector;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetPosition(const FVector2D& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Position, InValue); }
	const FVector2D& GetPosition() const { return Position; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 AddsAlive = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetAddsAlive(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(AddsAlive, InValue); }
	const int32& GetAddsAlive() const { return AddsAlive; }

};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIMinimapViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TSoftObjectPtr<UTexture2D> PlanTexture;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetPlanTexture(const TSoftObjectPtr<UTexture2D>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(PlanTexture, InValue); }
	const TSoftObjectPtr<UTexture2D>& GetPlanTexture() const { return PlanTexture; }

	/** World XY bounds covered by the plan texture. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FVector2D BoundsMin = FVector2D(-2500.f, -2500.f);
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetBoundsMin(const FVector2D& InValue) { UE_MVVM_SET_PROPERTY_VALUE(BoundsMin, InValue); }
	const FVector2D& GetBoundsMin() const { return BoundsMin; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FVector2D BoundsMax = FVector2D(2500.f, 2500.f);
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetBoundsMax(const FVector2D& InValue) { UE_MVVM_SET_PROPERTY_VALUE(BoundsMax, InValue); }
	const FVector2D& GetBoundsMax() const { return BoundsMax; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FVector2D PlayerPosition = FVector2D::ZeroVector;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetPlayerPosition(const FVector2D& InValue) { UE_MVVM_SET_PROPERTY_VALUE(PlayerPosition, InValue); }
	const FVector2D& GetPlayerPosition() const { return PlayerPosition; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float PlayerYaw = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetPlayerYaw(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(PlayerYaw, InValue); }
	const float& GetPlayerYaw() const { return PlayerYaw; }

	/** Updated at 10–15 Hz. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<FCoreKitBasicUIMinimapEntity> Entities;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetEntities(const TArray<FCoreKitBasicUIMinimapEntity>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Entities, InValue); }
	const TArray<FCoreKitBasicUIMinimapEntity>& GetEntities() const { return Entities; }

};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIRunSummaryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bVictory = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbVictory(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bVictory, InValue); }
	const bool& GetbVictory() const { return bVictory; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float TotalSeconds = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTotalSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(TotalSeconds, InValue); }
	const float& GetTotalSeconds() const { return TotalSeconds; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float BossSeconds = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetBossSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(BossSeconds, InValue); }
	const float& GetBossSeconds() const { return BossSeconds; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Kills = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetKills(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Kills, InValue); }
	const int32& GetKills() const { return Kills; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Level = 1;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetLevel(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Level, InValue); }
	const int32& GetLevel() const { return Level; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Dashes = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetDashes(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Dashes, InValue); }
	const int32& GetDashes() const { return Dashes; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float DeathSeconds = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetDeathSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(DeathSeconds, InValue); }
	const float& GetDeathSeconds() const { return DeathSeconds; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 DeathStage = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetDeathStage(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(DeathStage, InValue); }
	const int32& GetDeathStage() const { return DeathStage; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float ShortBySeconds = 0.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetShortBySeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ShortBySeconds, InValue); }
	const float& GetShortBySeconds() const { return ShortBySeconds; }

	/** "RANGED DRONE" */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText KillerType;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetKillerType(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(KillerType, InValue); }
	const FText& GetKillerType() const { return KillerType; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText Tip;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTip(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Tip, InValue); }
	const FText& GetTip() const { return Tip; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<FCoreKitBasicUICardData> FinalCards;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetFinalCards(const TArray<FCoreKitBasicUICardData>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(FinalCards, InValue); }
	const TArray<FCoreKitBasicUICardData>& GetFinalCards() const { return FinalCards; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 TotalCopies = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTotalCopies(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(TotalCopies, InValue); }
	const int32& GetTotalCopies() const { return TotalCopies; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FText Archetype;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetArchetype(const FText& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Archetype, InValue); }
	const FText& GetArchetype() const { return Archetype; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	int32 Serial = 0;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetSerial(const int32& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Serial, InValue); }
	const int32& GetSerial() const { return Serial; }

	/** Run timeline: stage starts, level ups, boss inbound/spawn, death or victory (added by UI-1). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<FCoreKitBasicUITimelineMark> Timeline;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetTimeline(const TArray<FCoreKitBasicUITimelineMark>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(Timeline, InValue); }
	const TArray<FCoreKitBasicUITimelineMark>& GetTimeline() const { return Timeline; }

	/** Stage boundaries for the timeline bar (start of each stage, plus the end; added by UI-1). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	TArray<float> StageStarts;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetStageStarts(const TArray<float>& InValue) { UE_MVVM_SET_PROPERTY_VALUE(StageStarts, InValue); }
	const TArray<float>& GetStageStarts() const { return StageStarts; }

	/** Seconds into the boss fight when the boss spawned (-1 if never; added by UI-1). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float BossSpawnSeconds = -1.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetBossSpawnSeconds(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(BossSpawnSeconds, InValue); }
	const float& GetBossSpawnSeconds() const { return BossSpawnSeconds; }

};


/** Player preferences the widgets react to (written by UCoreKitBasicUIUserSettings::Apply). */
UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIPreferencesViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 0.75..1.25 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float HudScale = 1.f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetHudScale(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(HudScale, InValue); }
	const float& GetHudScale() const { return HudScale; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	FLinearColor CrosshairColor = FLinearColor::White;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetCrosshairColor(const FLinearColor& InValue) { UE_MVVM_SET_PROPERTY_VALUE(CrosshairColor, InValue); }
	const FLinearColor& GetCrosshairColor() const { return CrosshairColor; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bReducedMotion = false;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbReducedMotion(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bReducedMotion, InValue); }
	const bool& GetbReducedMotion() const { return bReducedMotion; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bLowHealthHeartbeat = true;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbLowHealthHeartbeat(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bLowHealthHeartbeat, InValue); }
	const bool& GetbLowHealthHeartbeat() const { return bLowHealthHeartbeat; }

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	bool bDamageNumbers = true;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetbDamageNumbers(const bool& InValue) { UE_MVVM_SET_PROPERTY_VALUE(bDamageNumbers, InValue); }
	const bool& GetbDamageNumbers() const { return bDamageNumbers; }

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CoreKitBasicUI|UI")
	float ScreenShake = 0.6f;
	UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI")
	void SetScreenShake(const float& InValue) { UE_MVVM_SET_PROPERTY_VALUE(ScreenShake, InValue); }
	const float& GetScreenShake() const { return ScreenShake; }

};
