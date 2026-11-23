#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalLayer.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Actor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIArcWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIEnemySignalComponent.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalSubsystem.h"
#include "CoreKitBasicUI/Widgets/World/CoreKitBasicUIWorldSignalTypes.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIWorldSignals"

namespace
{
	constexpr float RingLabelGap = 9.f;
	/** The ring entry is a fixed 400 px box centred on the drone; parts are placed around its centre. */
	constexpr float RingBox = 400.f;
	constexpr float RingLabelHeight = 20.f;
	/** Ranged-drone lock-on UI is 60% smaller; boss signals retain their authored scale. */
	constexpr float RangedLockOnScale = 0.4f;
	constexpr float EdgeInset = 96.f;
	constexpr float PlateTop = 476.f;
	constexpr float PlateStep = 48.f;

	UCanvasPanelSlot* CanvasSlot(UWidget* Widget)
	{
		return Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Health bar

void UCoreKitBasicUIEnemyHealthBarWidget::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Root"));
	WidgetTree->RootWidget = Root;
	Track = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.7f)), TEXT("Track"));
	ChunkImage = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::White), TEXT("Chunk"));
	FillImage = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Orange), TEXT("Fill"));
	CoreKitBasicUIBuild::Place(Root, Track, FAnchors(0.f), FVector2D::ZeroVector, FVector2D(Width, Height));
	CoreKitBasicUIBuild::Place(Root, ChunkImage, FAnchors(0.f), FVector2D::ZeroVector, FVector2D(Width, Height), FVector2D::ZeroVector, 1);
	CoreKitBasicUIBuild::Place(Root, FillImage, FAnchors(0.f), FVector2D::ZeroVector, FVector2D(Width, Height), FVector2D::ZeroVector, 2);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCoreKitBasicUIEnemyHealthBarWidget::SetValues(float Fill, float Chunk)
{
	if (UCanvasPanelSlot* S = CanvasSlot(FillImage))
	{
		S->SetSize(FVector2D(Width * FMath::Clamp(Fill, 0.f, 1.f), Height));
	}
	if (UCanvasPanelSlot* S = CanvasSlot(ChunkImage))
	{
		S->SetSize(FVector2D(Width * FMath::Clamp(Chunk, 0.f, 1.f), Height));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Charge ring

void UCoreKitBasicUIChargeRingWidget::BuildTree()
{
	Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Root"));
	WidgetTree->RootWidget = Root;
	Glow = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.55f)), TEXT("Glow"));
	Conic = WidgetTree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), TEXT("Conic"));
	Conic->SetArcColor(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.28f));
	Ring = WidgetTree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), TEXT("Ring"));
	Ring->SetArcColor(CoreKitBasicUIColor::Orange);
	Ring->SetThickness(4.f);
	CoreKitBasicUIBuild::FTextStyle LabelStyle = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(11.f), 700, CoreKitBasicUIColor::Soot, 0.16f);
	UTextBlock* LabelText = CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Charging", "CHARGING"), LabelStyle, TEXT("LabelText"));
	Label = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Orange), FMargin(6.f, 3.f), TEXT("Label"));
	Cast<UBorder>(Label)->SetContent(LabelText);
	const FVector2D Mid(RingBox * 0.5f);
	CoreKitBasicUIBuild::Place(Root, Glow, FAnchors(0.f), Mid, FVector2D(280.f), FVector2D(0.5f), 0);
	CoreKitBasicUIBuild::Place(Root, Conic, FAnchors(0.f), Mid, FVector2D(280.f), FVector2D(0.5f), 1);
	CoreKitBasicUIBuild::Place(Root, Ring, FAnchors(0.f), Mid, FVector2D(280.f), FVector2D(0.5f), 2);
	UCanvasPanelSlot* LabelSlot = CoreKitBasicUIBuild::Place(Root, Label, FAnchors(0.f), Mid + FVector2D(0.f, 140.f + RingLabelGap), FVector2D::ZeroVector, FVector2D(0.5f, 0.f), 3);
	LabelSlot->SetAutoSize(true);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCoreKitBasicUIChargeRingWidget::SetRing(float Diameter, float Progress, float PulseAlpha)
{
	// The widget's own canvas slot in the layer is centred on the drone; children are placed around the box centre.
	Diameter = FMath::Min(Diameter, RingBox * 0.78f);
	if (!FMath::IsNearlyEqual(Diameter, LastDiameter, 0.5f))
	{
		LastDiameter = Diameter;
		const FVector2D Size(Diameter);
		if (UCanvasPanelSlot* S = CanvasSlot(Glow)) { S->SetSize(Size * 1.25f); }
		if (UCanvasPanelSlot* S = CanvasSlot(Ring)) { S->SetSize(Size); }
		if (UCanvasPanelSlot* S = CanvasSlot(Conic)) { S->SetSize(Size); }
		if (UCanvasPanelSlot* S = CanvasSlot(Label)) { S->SetPosition(FVector2D(RingBox * 0.5f, RingBox * 0.5f + Diameter * 0.5f + RingLabelGap)); }
		// Conic fill: a thick arc from the centre to the ring (SCoreKitBasicUIArc draws at radius = size / 2 - thickness / 2).
		Conic->SetThickness(FMath::Max(1.f, Diameter * 0.5f - 4.f));
	}
	Conic->SetProgress(FMath::Clamp(Progress, 0.f, 1.f));
	SetRenderOpacity(FMath::Clamp(PulseAlpha, 0.f, 1.f));
}

// ---------------------------------------------------------------------------------------------------------------
// Off-screen lock-on plate

void UCoreKitBasicUILockOnPlateWidget::BuildTree()
{
	UBorder* Plate = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(14, CoreKitBasicUIColor::SootA(0.72f)), FMargin(18.f, 0.f, 14.f, 0.f), TEXT("Plate"));
	WidgetTree->RootWidget = Plate;
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("Row"));
	Plate->SetContent(CoreKitBasicUIBuild::Box(WidgetTree, Row, 0.f, Height, TEXT("PlateHeight")));
	if (USizeBox* HeightBox = Cast<USizeBox>(Plate->GetContent()))
	{
		HeightBox->ClearWidthOverride();
	}

	UOverlay* MiniRing = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("MiniRing"));
	Mini = WidgetTree->ConstructWidget<UCoreKitBasicUIArcWidget>(UCoreKitBasicUIArcWidget::StaticClass(), TEXT("Mini"));
	Mini->SetArcColor(CoreKitBasicUIColor::Orange);
	Mini->SetTrackColor(CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Orange, 0.2f));
	Mini->SetThickness(12.f);
	CoreKitBasicUIBuild::AddO(MiniRing, CoreKitBasicUIBuild::Box(WidgetTree, Mini, 24.f, 24.f));
	CoreKitBasicUIBuild::AddO(MiniRing, CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_UI_Circle"), CoreKitBasicUIColor::Soot, 16.f), HAlign_Center, VAlign_Center);
	CoreKitBasicUIBuild::AddH(Row, MiniRing, FMargin(0.f, 0.f, 10.f, 0.f));
	CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("LockingOn", "LOCKING ON"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(11.f), 700, CoreKitBasicUIColor::Orange, 0.16f), TEXT("Label")),
		FMargin(0.f, 0.f, 10.f, 0.f));
	Chevrons = CoreKitBasicUIBuild::Icon(WidgetTree, TEXT("T_Icon_Chevrons"), CoreKitBasicUIColor::Orange, 20.f, TEXT("Chevrons"));
	CoreKitBasicUIBuild::AddH(Row, Chevrons);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCoreKitBasicUILockOnPlateWidget::SetPlate(float Progress, bool bRightEdge)
{
	Mini->SetProgress(FMath::Clamp(Progress, 0.f, 1.f));
	if (bRight != bRightEdge)
	{
		bRight = bRightEdge;
		// Chevrons point toward the threat (right edge: right, left edge: left).
		Chevrons->SetRenderScale(FVector2D(bRight ? 1.f : -1.f, 1.f));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Layer

void UCoreKitBasicUIWorldSignalLayer::BuildTree()
{
	Canvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("WorldSignals"));
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

UCoreKitBasicUIWorldSignalLayer::FEntry& UCoreKitBasicUIWorldSignalLayer::FindOrAdd(UCoreKitBasicUIEnemySignalComponent* Signal)
{
	if (FEntry* Found = Entries.Find(Signal))
	{
		return *Found;
	}
	FEntry& Entry = Entries.Add(Signal);
	APlayerController* Owner = GetOwningPlayer();
	Entry.Bar = CreateWidget<UCoreKitBasicUIEnemyHealthBarWidget>(Owner, UCoreKitBasicUIEnemyHealthBarWidget::StaticClass());
	Entry.Ring = CreateWidget<UCoreKitBasicUIChargeRingWidget>(Owner, UCoreKitBasicUIChargeRingWidget::StaticClass());
	Entry.Plate = CreateWidget<UCoreKitBasicUILockOnPlateWidget>(Owner, UCoreKitBasicUILockOnPlateWidget::StaticClass());
	// Scale the complete presentation together, including glow, line thickness and label.
	// Keep local arc geometry unchanged so the existing safe-draw guards still apply.
	const float LockOnScale = Signal->IsBoss() ? 1.f : RangedLockOnScale;
	Entry.Ring->SetRenderTransformPivot(FVector2D(0.5f));
	Entry.Ring->SetRenderScale(FVector2D(LockOnScale));
	Entry.Plate->SetRenderScale(FVector2D(LockOnScale));
	CoreKitBasicUIBuild::Place(Canvas, Entry.Bar, FAnchors(0.f), FVector2D::ZeroVector, FVector2D(UCoreKitBasicUIEnemyHealthBarWidget::Width, UCoreKitBasicUIEnemyHealthBarWidget::Height), FVector2D(0.5f, 1.f), 1);
	CoreKitBasicUIBuild::Place(Canvas, Entry.Ring, FAnchors(0.f), FVector2D::ZeroVector, FVector2D(RingBox), FVector2D(0.5f), 0);
	UCanvasPanelSlot* PlateSlot = CoreKitBasicUIBuild::Place(Canvas, Entry.Plate, FAnchors(0.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(1.f, 0.f), 2);
	PlateSlot->SetAutoSize(true);
	Owned.Add(Entry.Bar);
	Owned.Add(Entry.Ring);
	Owned.Add(Entry.Plate);
	Hide(Entry);
	return Entry;
}

void UCoreKitBasicUIWorldSignalLayer::Hide(FEntry& Entry) const
{
	for (UWidget* Widget : { static_cast<UWidget*>(Entry.Bar), static_cast<UWidget*>(Entry.Ring), static_cast<UWidget*>(Entry.Plate) })
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCoreKitBasicUIWorldSignalLayer::TickMotion(float RealDeltaSeconds)
{
	PulseTime += RealDeltaSeconds;
	APlayerController* Controller = GetOwningPlayer();
	UWorld* World = GetWorld();
	UCoreKitBasicUIWorldSignalSubsystem* Subsystem = World ? World->GetSubsystem<UCoreKitBasicUIWorldSignalSubsystem>() : nullptr;
	if (!Controller || !Subsystem || !Canvas)
	{
		return;
	}
	for (TPair<TWeakObjectPtr<UCoreKitBasicUIEnemySignalComponent>, FEntry>& Pair : Entries)
	{
		Pair.Value.bSeen = false;
	}

	FVector Eye;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(Eye, ViewRotation);
	const float ViewScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	const FVector2D ViewSize = UWidgetLayoutLibrary::GetViewportSize(this) / ViewScale;
	const float Fov = Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetFOVAngle() : 90.f;
	const float FocalPx = ViewSize.X * 0.5f / FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(Fov, 10.f, 170.f) * 0.5f));
	const UCoreKitBasicUIWorldSignalSettings* Settings = GetDefault<UCoreKitBasicUIWorldSignalSettings>();
	int32 PlatesRight = 0;
	int32 PlatesLeft = 0;

	auto Project = [&](const FVector& Location, FVector2D& Out) -> bool
	{
		if (FVector::DotProduct(Location - Eye, ViewRotation.Vector()) <= 0.f)
		{
			return false;
		}
		return UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller, Location, Out, true);
	};
	auto OnScreen = [&](const FVector2D& P, float Margin) -> bool
	{
		return P.X >= -Margin && P.Y >= -Margin && P.X <= ViewSize.X + Margin && P.Y <= ViewSize.Y + Margin;
	};

	for (const TWeakObjectPtr<UCoreKitBasicUIEnemySignalComponent>& Weak : Subsystem->GetSignals())
	{
		UCoreKitBasicUIEnemySignalComponent* Signal = Weak.Get();
		AActor* Enemy = Signal ? Signal->GetEnemy() : nullptr;
		if (!Enemy)
		{
			continue;
		}
		FEntry& Entry = FindOrAdd(Signal);
		Entry.bSeen = true;
		Signal->TickPresentation(RealDeltaSeconds);
		const FVector Center = Enemy->GetActorLocation();

		// Damaged-enemy bar.
		const float BarOpacity = Signal->WantsHealthBar() ? CoreKitBasicUIWorldSignals::HealthBarOpacity(Signal->GetSecondsSinceHit(), Settings->HealthBarLinger) : 0.f;
		FVector2D BarPoint;
		if (BarOpacity > 0.f && Project(Center + FVector(0.f, 0.f, Signal->GetTopOffset() + Settings->HealthBarHeight), BarPoint) && OnScreen(BarPoint, 0.f))
		{
			Entry.Bar->SetValues(Signal->GetBar().Fill, Signal->GetBar().Chunk);
			Entry.Bar->SetRenderOpacity(BarOpacity);
			CanvasSlot(Entry.Bar)->SetPosition(BarPoint);
			Entry.Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Entry.Bar->SetVisibility(ESlateVisibility::Collapsed);
		}

		// CHARGING ring on screen, LOCKING ON plate on the edge when the drone is off screen or behind.
		if (!Signal->IsCharging())
		{
			Entry.Ring->SetVisibility(ESlateVisibility::Collapsed);
			Entry.Plate->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		const float Progress = Signal->GetChargeProgress();
		FVector2D RingPoint;
		if (Project(Center, RingPoint) && OnScreen(RingPoint, -24.f))
		{
			const float Diameter = CoreKitBasicUIWorldSignals::ChargeRingDiameter(Settings->ChargeRingWorldRadius * (Signal->IsBoss() ? 2.f : 1.f),
				FVector::Distance(Eye, Center), FocalPx, Settings->ChargeRingMinPx, Settings->ChargeRingMaxPx);
			Entry.Ring->SetRing(Diameter, Progress, 1.f);
			CanvasSlot(Entry.Ring)->SetPosition(RingPoint);
			Entry.Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
			Entry.Plate->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			const FVector Local = ViewRotation.UnrotateVector(Center - Eye);
			const bool bRight = CoreKitBasicUIWorldSignals::EdgeSide(FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X))) > 0.f;
			int32& Stack = bRight ? PlatesRight : PlatesLeft;
			UCanvasPanelSlot* PlateSlot = CanvasSlot(Entry.Plate);
			PlateSlot->SetAlignment(FVector2D(bRight ? 1.f : 0.f, 0.f));
			// Scaling stays anchored at the screen edge instead of moving the smaller plate inward.
			Entry.Plate->SetRenderTransformPivot(FVector2D(bRight ? 1.f : 0.f, 0.f));
			PlateSlot->SetPosition(FVector2D(bRight ? ViewSize.X - EdgeInset : EdgeInset, PlateTop + PlateStep * Stack));
			++Stack;
			Entry.Plate->SetPlate(Progress, bRight);
			Entry.Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
			Entry.Ring->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Drop entries of destroyed enemies.
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		if (!It->Value.bSeen)
		{
			for (UWidget* Widget : { static_cast<UWidget*>(It->Value.Bar), static_cast<UWidget*>(It->Value.Ring), static_cast<UWidget*>(It->Value.Plate) })
			{
				if (Widget)
				{
					Widget->RemoveFromParent();
					Owned.Remove(Widget);
				}
			}
			It.RemoveCurrent();
		}
	}
}

#undef LOCTEXT_NAMESPACE
