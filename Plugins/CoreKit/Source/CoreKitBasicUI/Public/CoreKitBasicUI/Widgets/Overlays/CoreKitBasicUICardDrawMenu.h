#pragma once

#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"
#include "CoreKitBasicUICardDrawMenu.generated.h"

class UBackgroundBlur;
class UBorder;
class UCanvasPanel;
class UCoreKitButton;
class UCoreKitBasicUICardWidget;
class UCoreKitBasicUIKeycap;
class UCoreKitBasicUIProgressionViewModel;
class UCoreKitBasicUIBuildViewModel;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UWidget;

/**
 * Level Up · Card Draw overlay (Docs/UI/Spec/04-Overlays.md, artboards CardArt_Draw + Motion_LevelUp), pushed on the
 * "Menu" layer by UCoreKitBasicUIScreenRouter while an offer is open. Runs the Motion_LevelUp sequence (flash, title slam, world
 * freeze, deal), lets the player choose with click, 1/2/3 or gamepad focus + A after a 400 ms lock (D6), then plays the
 * pick (winner lifts, losers drop, winner flies to the hand) and commits once through
 * UCoreKitBasicUIProgressionViewModel::RequestChooseCard when the card reaches the hand. Chained offers re-deal in place.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUICardDrawMenu : public UCoreKitBasicUIMenuWidget
{
	GENERATED_BODY()

public:
	UCoreKitBasicUICardDrawMenu(const FObjectInitializer& ObjectInitializer);

	/** Overrides the ViewModels (tests); by default they come from the UI context. */
	void SetViewModels(UCoreKitBasicUIProgressionViewModel* InProgression, UCoreKitBasicUIBuildViewModel* InBuild);

	/** Choose an offered card (click, 1/2/3, gamepad A). Returns false while locked or after a pick. */
	bool ChooseCard(int32 Index);

	/** True once the sequence finished and the menu asked to close. */
	bool IsFinished() const { return bFinished; }
	const FCoreKitBasicUICardDrawFlow& GetFlow() const { return Flow; }

	/** Sequence times (s) of Motion_LevelUp. */
	static constexpr float TimeXPFill = 0.12f;
	static constexpr float TimeFlash = 0.57f;
	static constexpr float TimeTitle = 0.72f;
	static constexpr float TimeDeal = 1.12f;
	static constexpr float TimeChoose = 1.78f;
	/** After the pick: lift 250 ms, flight 500 ms; the commit happens on arrival. */
	static constexpr float PickFlyStart = 0.25f;
	static constexpr float PickArrive = 0.75f;
	static constexpr float UnfreezeSeconds = 0.25f;

protected:
	void BuildTree() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void TickMotion(float DeltaSeconds) override;
	UWidget* GetInitialFocus_Implementation() const override;
	bool HandleReturn_Implementation() override;
	bool HandleMenuKey_Implementation(FKey Key) override;
	FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

private:
	struct FCardSlot
	{
		TObjectPtr<UCoreKitButton> Button;
		TObjectPtr<UCoreKitBasicUICardWidget> Card;
		FCoreKitBasicUITween X, Y, Angle, Scale, Opacity;
		float Base = 0.f;
	};

	UFUNCTION()
	void HandleCardActivated(UCoreKitButton* Button);
	UFUNCTION()
	void HandleCardSelected(UCoreKitButton* Button);

	UCoreKitBasicUIProgressionViewModel* Progression() const;
	UCoreKitBasicUIBuildViewModel* Build() const;
	void StartOffer(bool bChained);
	void BuildCards();
	void RefreshHeader();
	void RefreshHand();
	void RefreshKeyHints();
	void UpdateSequence(float DeltaSeconds);
	void UpdateCards(float DeltaSeconds);
	void UpdateTilt();
	void BeginPick(int32 Index);
	void Commit();
	void BeginUnfreeze();
	void SetStatus(bool bDrawn, const FText& CardName);
	void SetFreeze(float Alpha);
	FVector2D SlotCenter(int32 Index) const;

	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIProgressionViewModel> ProgressionOverride;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIBuildViewModel> BuildOverride;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UBackgroundBlur> Blur;
	UPROPERTY(Transient) TObjectPtr<UWidget> VeilGroup;
	UPROPERTY(Transient) TObjectPtr<UImage> Flash;
	UPROPERTY(Transient) TObjectPtr<UWidget> IntroSlash;
	UPROPERTY(Transient) TObjectPtr<UWidget> ChooseSlash;
	UPROPERTY(Transient) TObjectPtr<UWidget> IntroTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> IntroLevel;
	UPROPERTY(Transient) TObjectPtr<UWidget> Header;
	UPROPERTY(Transient) TObjectPtr<UWidget> ChipRow;
	UPROPERTY(Transient) TObjectPtr<UWidget> DrawOneTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelChip;
	UPROPERTY(Transient) TObjectPtr<UWidget> QueuedChip;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QueuedChipText;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> CardLayer;
	UPROPERTY(Transient) TObjectPtr<UWidget> Footer;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> Hand;
	UPROPERTY(Transient) TObjectPtr<UBorder> StatusPlate;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIKeycap>> FooterKeys;
	UPROPERTY(Transient) TArray<TObjectPtr<UObject>> KeepAlive;

	TArray<FCardSlot> Slots;
	TArray<FCoreKitBasicUICardData> Offer;
	FCoreKitBasicUICardDrawFlow Flow;
	FCoreKitBasicUITween FreezeTween;
	FCoreKitBasicUITween TitleScale, TitleOpacity, TitleRise;
	FCoreKitBasicUITween IntroSlashScale, IntroSlashOpacity, ChooseSlashOpacity;
	FCoreKitBasicUITween ChipsOpacity, DrawOneOpacity, FooterOpacity;
	FVector2D StickTilt = FVector2D::ZeroVector;
	float Clock = 0.f;
	float FlashClock = -1.f;
	float SinceCommit = -1.f;
	int32 DealtSerial = INDEX_NONE;
	int32 HandSerial = INDEX_NONE;
	int32 DealCuesPlayed = 0;
	int32 HoveredCue = INDEX_NONE;
	uint8 FiredSteps = 0;
	int32 PadFocusIndex = INDEX_NONE;
	bool bGamepadKeys = false;
	bool bKeysDirty = true;
	bool bUnfreezing = false;
	bool bFinished = false;
	bool bFrozenSet = false;
};
