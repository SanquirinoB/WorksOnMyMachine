#pragma once

#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIWorldSignalLayer.generated.h"

class UCanvasPanel;
class UCoreKitBasicUIArcWidget;
class UCoreKitBasicUIEnemySignalComponent;
class UImage;
class UTextBlock;

/** Damaged-enemy health bar: 100 x 6 px, Soot 0.7 track, orange fill, white lag chunk (Docs/UI/Spec/03-HUDEvents.md). */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIEnemyHealthBarWidget : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	static constexpr float Width = 100.f;
	static constexpr float Height = 6.f;
	void SetValues(float Fill, float Chunk);

protected:
	void BuildTree() override;

private:
	UPROPERTY(Transient) TObjectPtr<UImage> Track;
	UPROPERTY(Transient) TObjectPtr<UImage> ChunkImage;
	UPROPERTY(Transient) TObjectPtr<UImage> FillImage;
};

/** CHARGING ring around a telegraphing drone: 4 px orange ring with glow, conic fill (alpha 0.28) and label. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIChargeRingWidget : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	void SetRing(float Diameter, float Progress, float PulseAlpha);

protected:
	void BuildTree() override;

private:
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Root;
	UPROPERTY(Transient) TObjectPtr<UImage> Glow;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> Conic;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> Ring;
	UPROPERTY(Transient) TObjectPtr<UWidget> Label;
	float LastDiameter = -1.f;
};

/** Off-screen lock-on plate on the screen edge: arrow notch, conic mini ring, "LOCKING ON", chevrons. 40 px high. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUILockOnPlateWidget : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	static constexpr float Height = 40.f;
	void SetPlate(float Progress, bool bRightEdge);

protected:
	void BuildTree() override;

private:
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIArcWidget> Mini;
	UPROPERTY(Transient) TObjectPtr<UImage> Chevrons;
	UPROPERTY(Transient) TObjectPtr<UImage> Notch;
	bool bRight = true;
};

/**
 * Screen layer under the HUD that projects the world signals of every registered enemy each frame (real time, also
 * while paused). Entries are pooled per enemy; nothing is created per frame.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIWorldSignalLayer : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

protected:
	void BuildTree() override;
	void TickMotion(float RealDeltaSeconds) override;

private:
	struct FEntry
	{
		TObjectPtr<UCoreKitBasicUIEnemyHealthBarWidget> Bar;
		TObjectPtr<UCoreKitBasicUIChargeRingWidget> Ring;
		TObjectPtr<UCoreKitBasicUILockOnPlateWidget> Plate;
		bool bSeen = false;
	};

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	/** Strong references for the pooled entry widgets (also owned by the canvas). */
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Owned;
	TMap<TWeakObjectPtr<UCoreKitBasicUIEnemySignalComponent>, FEntry> Entries;
	float PulseTime = 0.f;

	FEntry& FindOrAdd(UCoreKitBasicUIEnemySignalComponent* Signal);
	void Hide(FEntry& Entry) const;
};
