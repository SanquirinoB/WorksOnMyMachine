#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudBuild.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Components/Border.h"
#include "Widgets/HUD/CoreKitBasicUIHudCommon.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIHud"

using namespace CoreKitBasicUIHudBuild;

namespace
{
	bool SameCards(const TArray<FCoreKitBasicUICardData>& A, const TArray<FCoreKitBasicUICardData>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!(A[Index] == B[Index]))
			{
				return false;
			}
		}
		return true;
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Hand

void UCoreKitBasicUIHudBuildHand::BuildTree()
{
	UOverlay* Root = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("HandStack"));
	BlockBox = CoreKitBasicUIBuild::Box(WidgetTree, Root, 600.f, 88.f, TEXT("BuildHand"));
	WidgetTree->RootWidget = BlockBox;
	UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("HandRow"));
	CoreKitBasicUIBuild::AddO(Root, Row, HAlign_Center, VAlign_Fill);

	Key = WidgetTree->ConstructWidget<UCoreKitBasicUIHudKeycap>(UCoreKitBasicUIHudKeycap::StaticClass(), TEXT("InspectKey"));
	// Current binding of Inspect build; spec defaults (Tab / View) while the action has no key for the device.
	Key->SetFallbackKeys(EKeys::Tab, EKeys::Gamepad_Special_Left);
	if (UInputAction* Inspect = CoreKitBasicUIHudUtil::FindInputAction(TEXT("IA_InspectBuild")))
	{
		Key->SetAction(Inspect);
	}
	else
	{
		Key->SetKey(EKeys::Tab);
	}
	CoreKitBasicUIBuild::AddH(Row, Key, FMargin(0.f, 0.f, 16.f, 24.f), VAlign_Bottom);
	Fan = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Fan"));
	FanBox = CoreKitBasicUIBuild::Box(WidgetTree, Fan, 0.f, 88.f, TEXT("FanBox"));
	CoreKitBasicUIBuild::AddH(Row, FanBox, FMargin(0.f), VAlign_Bottom);
	Lift.Snap(0.f);
}

void UCoreKitBasicUIHudBuildHand::RebuildCards(const TArray<FCoreKitBasicUICardData>& InCards)
{
	// Reuse card widgets; create the missing ones.
	while (CardWidgets.Num() < InCards.Num())
	{
		UCoreKitBasicUICardWidget* Card = WidgetTree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass(), *FString::Printf(TEXT("HandCard%d"), CardWidgets.Num()));
		Card->SetVariant(ECoreKitBasicUICardVariant::Mini);
		USizeBox* CardBox = CoreKitBasicUIBuild::Box(WidgetTree, Card, 52.f, 72.f);
		// Fan origin 50 % 130 %.
		CardBox->SetRenderTransformPivot(FVector2D(0.5f, 1.3f));
		CoreKitBasicUIBuild::Place(Fan, CardBox, FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(52.f, 72.f), FVector2D::ZeroVector, CardWidgets.Num());
		CardWidgets.Add(Card);
		CardBoxes.Add(CardBox);
	}
	for (int32 Index = 0; Index < CardWidgets.Num(); ++Index)
	{
		const bool bUsed = InCards.IsValidIndex(Index);
		Show(CardBoxes[Index], bUsed);
		if (bUsed && (!Shown.IsValidIndex(Index) || !(Shown[Index] == InCards[Index])))
		{
			CardWidgets[Index]->SetCard(InCards[Index]);
		}
	}
	Shown = InCards;
	const int32 Count = Shown.Num();
	BlockBox->SetWidthOverride(CoreKitBasicUIHudLogic::HandBlockWidth(Count));
	FanBox->SetWidthOverride(FMath::Max(CoreKitBasicUIHudLogic::FanWidth(Count), 1.f));

	TArray<FString> Names;
	for (const FCoreKitBasicUICardData& Card : Shown)
	{
		Names.Add(Card.bSpecial ? Card.Name.ToString() : FString::Printf(TEXT("%s x%d"), *Card.Name.ToString(), Card.Copies));
	}
	CoreKitBasicUIHudUtil::SetAccessibleText(this, FText::FromString(TEXT("Build: ") + FString::Join(Names, TEXT(", "))));
}

void UCoreKitBasicUIHudBuildHand::OnCardChosen(const FCoreKitBasicUICardData& Card, bool bStacked)
{
	StartLanding(Card.Id, bStacked);
}

void UCoreKitBasicUIHudBuildHand::StartLanding(FName CardId, bool bStacked)
{
	if (CardId.IsNone())
	{
		return;
	}
	// Same pick reported twice (CardChosen event and the Build ViewModel serial): keep the first.
	if (CardId == AnimId && Clock - AnimStartClock < 2.0)
	{
		return;
	}
	if (bNewOn)
	{
		const int32 Previous = Shown.IndexOfByPredicate([this](const FCoreKitBasicUICardData& Data) { return Data.Id == AnimId; });
		if (CardWidgets.IsValidIndex(Previous))
		{
			CardWidgets[Previous]->SetNew(false);
		}
	}
	AnimId = CardId;
	bAnimStacked = bStacked;
	bAnimating = true;
	// Card Draw commits the pick when its flying card reaches the hand: land at once (no second flight wait).
	AnimTime = CoreKitBasicUIHudTiming::CardFlight;
	AnimStartClock = Clock;
	bLandCue = false;
	bNewOn = false;
	NewestId = CardId;
	Lift.Snap(8.f);
}

FVector2D UCoreKitBasicUIHudBuildHand::GetNewestSlotCenter() const
{
	const int32 Index = Shown.IndexOfByPredicate([this](const FCoreKitBasicUICardData& Data) { return Data.Id == NewestId; });
	const int32 Count = Shown.Num();
	const TArray<FCoreKitBasicUIFanSlot> Slots = CoreKitBasicUIHudLogic::FanLayout(Count, Index);
	const float BlockWidth = CoreKitBasicUIHudLogic::HandBlockWidth(Count);
	const float FanLeft = BlockWidth * 0.5f + (CoreKitBasicUIHudLogic::FanWidth(Count) + 16.f + 40.f) * 0.5f - CoreKitBasicUIHudLogic::FanWidth(Count);
	return Slots.IsValidIndex(Index) ? FVector2D(FanLeft + Slots[Index].X + 26.f, 16.f + 36.f + Slots[Index].OffsetY) : FVector2D(BlockWidth * 0.5f, 52.f);
}

void UCoreKitBasicUIHudBuildHand::LayoutCards(float Dt)
{
	const int32 NewestIndex = Shown.IndexOfByPredicate([this](const FCoreKitBasicUICardData& Data) { return Data.Id == NewestId; });
	Lift.Tick(Dt);
	const TArray<FCoreKitBasicUIFanSlot> Slots = CoreKitBasicUIHudLogic::FanLayout(Shown.Num(), NewestIndex, Lift.Value);
	for (int32 Index = 0; Index < Slots.Num() && Index < CardBoxes.Num(); ++Index)
	{
		const FCoreKitBasicUIFanSlot& FanSlot = Slots[Index];
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CardBoxes[Index]->Slot))
		{
			CanvasSlot->SetPosition(FVector2D(FanSlot.X, 16.f));
		}
		float OffsetY = FanSlot.OffsetY;
		float Scale = 1.f;
		float Opacity = 1.f;
		if (bAnimating && Index == NewestIndex)
		{
			const float Motion = MotionAmount();
			if (!bAnimStacked)
			{
				if (AnimTime < CoreKitBasicUIHudTiming::CardFlight)
				{
					// Card Draw draws the flight; the slot waits empty.
					Opacity = 0.f;
				}
				else
				{
					const float A = FMath::Clamp((AnimTime - CoreKitBasicUIHudTiming::CardFlight) / CoreKitBasicUIHudTiming::CardLand, 0.f, 1.f);
					const float E = Ease(ECoreKitBasicUIMotion::Slam, A);
					OffsetY += FMath::Lerp(12.f, 0.f, E) * Motion;
					Scale = FMath::Lerp(1.f - 0.05f * Motion, 1.f, E);
				}
			}
			else if (AnimTime >= CoreKitBasicUIHudTiming::CardFlight)
			{
				// Stacked: the existing card pops (×N grows inside the card).
				const float A = FMath::Clamp((AnimTime - CoreKitBasicUIHudTiming::CardFlight) / CoreKitBasicUIHudTiming::CardLand, 0.f, 1.f);
				Scale = 1.f + 0.15f * (1.f - Ease(ECoreKitBasicUIMotion::Slam, A)) * Motion;
			}
		}
		CardBoxes[Index]->SetRenderOpacity(Opacity);
		SetXform(CardBoxes[Index], FVector2D(0.f, OffsetY), Scale, FanSlot.AngleDeg);
	}
}

void UCoreKitBasicUIHudBuildHand::TickMotion(float Dt)
{
	Clock += Dt;
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIBuildViewModel* Build = Ctx ? Ctx->GetBuild() : nullptr;
	if (!Build)
	{
		return;
	}
	if (!SameCards(Build->Cards, Shown))
	{
		const TArray<FCoreKitBasicUICardData> Previous = Shown;
		RebuildCards(Build->Cards);
		if (LastPickSerial != INDEX_NONE && Build->PickSerial != LastPickSerial && !Build->NewestCard.IsNone())
		{
			const bool bExisted = Previous.ContainsByPredicate([Build](const FCoreKitBasicUICardData& Data) { return Data.Id == Build->NewestCard; });
			StartLanding(Build->NewestCard, bExisted);
		}
	}
	else if (LastPickSerial != INDEX_NONE && Build->PickSerial != LastPickSerial && !Build->NewestCard.IsNone())
	{
		StartLanding(Build->NewestCard, true);
	}
	if (LastPickSerial == INDEX_NONE)
	{
		NewestId = Build->NewestCard;
	}
	LastPickSerial = Build->PickSerial;

	if (bAnimating)
	{
		AnimTime += Dt;
		const int32 Index = Shown.IndexOfByPredicate([this](const FCoreKitBasicUICardData& Data) { return Data.Id == AnimId; });
		if (!bLandCue && AnimTime >= CoreKitBasicUIHudTiming::CardFlight)
		{
			bLandCue = true;
			PlayHudCue(TEXT("Cards.Land"));
		}
		if (!bAnimStacked && !bNewOn && AnimTime >= CoreKitBasicUIHudTiming::CardNewTag && CardWidgets.IsValidIndex(Index))
		{
			// NEW tag pops at 700 ms; the card component draws the tag and the suit glow.
			bNewOn = true;
			CardWidgets[Index]->SetNew(true);
		}
		if (AnimTime >= CoreKitBasicUIHudTiming::CardGlowStart + CoreKitBasicUIHudTiming::CardGlow)
		{
			bAnimating = false;
			if (bNewOn && CardWidgets.IsValidIndex(Index))
			{
				CardWidgets[Index]->SetNew(false);
			}
			bNewOn = false;
			Lift.Start(Lift.Value, 0.f, ECoreKitBasicUIMotion::Settle);
		}
	}
	LayoutCards(Dt);
}

// ---------------------------------------------------------------------------------------------------------------
// TAB inspect panel

void UCoreKitBasicUIHudBuildInspector::BuildTree()
{
	UOverlay* Stack = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("InspectStack"));
	Panel = Stack;
	WidgetTree->RootWidget = CoreKitBasicUIBuild::Box(WidgetTree, Stack, 520.f, 0.f, TEXT("BuildInspector"));
	CoreKitBasicUIBuild::AddO(Stack, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Chamfer(12, CoreKitBasicUIColor::SootA(0.82f)), TEXT("InspectPlate")));
	// Magenta top border of 3 px (starts after the chamfer).
	CoreKitBasicUIBuild::AddO(Stack, CoreKitBasicUIBuild::Box(WidgetTree, Solid(WidgetTree, CoreKitBasicUIColor::Magenta), 0.f, 3.f), HAlign_Fill, VAlign_Top, FMargin(10.f, 0.f, 0.f, 0.f));

	UVerticalBox* Column = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("InspectColumn"));
	CoreKitBasicUIBuild::AddO(Stack, Column, HAlign_Fill, VAlign_Fill, FMargin(20.f, 14.f, 20.f, 10.f));
	UHorizontalBox* HeaderRow = CoreKitBasicUIBuild::Row(WidgetTree, TEXT("InspectHeader"));
	CoreKitBasicUIBuild::AddV(Column, HeaderRow, FMargin(0.f, 0.f, 0.f, 6.f));
	Header = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), M(11.f, 500, CoreKitBasicUIColor::Magenta, 0.2f), TEXT("BuildCount"));
	CoreKitBasicUIBuild::AddH(HeaderRow, Header, FMargin(0.f), VAlign_Center, HAlign_Left, 1.f);
	Archetype = CoreKitBasicUIBuild::Text(WidgetTree, FText::GetEmpty(), M(10.f, 500, CoreKitBasicUIColor::Steel, 0.14f), TEXT("Archetype"));
	CoreKitBasicUIBuild::AddH(HeaderRow, Archetype);
	Rows = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("InspectRows"));
	CoreKitBasicUIBuild::AddV(Column, Rows);
	SetRenderOpacity(0.f);
	Show(Panel, false);
}

void UCoreKitBasicUIHudBuildInspector::RebuildRows(const TArray<FCoreKitBasicUICardData>& InCards)
{
	Rows->ClearChildren();
	for (int32 Index = 0; Index < InCards.Num(); ++Index)
	{
		const FCoreKitBasicUICardData& Card = InCards[Index];
		UOverlay* RowStack = CoreKitBasicUIBuild::Overlay(WidgetTree);
		CoreKitBasicUIBuild::AddV(Rows, CoreKitBasicUIBuild::Box(WidgetTree, RowStack, 0.f, 30.f));
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddO(RowStack, Row);

		UCoreKitBasicUICardWidget* Thumb = WidgetTree->ConstructWidget<UCoreKitBasicUICardWidget>(UCoreKitBasicUICardWidget::StaticClass());
		Thumb->SetVariant(ECoreKitBasicUICardVariant::Thumb, 22.f);
		Thumb->SetCard(Card);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Box(WidgetTree, Thumb, 22.f, 22.f), FMargin(0.f, 0.f, 12.f, 0.f));

		UHorizontalBox* NameRow = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddH(NameRow, CoreKitBasicUIBuild::Text(WidgetTree, Card.Name.ToUpper(), D(20.f, 700)), FMargin(0.f, 0.f, 6.f, 0.f));
		if (Card.bSpecial)
		{
			UBorder* Chip = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta), FMargin(4.f, 1.f));
			Chip->SetContent(CoreKitBasicUIBuild::Text(WidgetTree, COREKIT_BASICUI_TEXT("Special", "SPECIAL"), M(10.f, 500, CoreKitBasicUIColor::Soot, 0.f)));
			CoreKitBasicUIBuild::AddH(NameRow, Chip);
		}
		else
		{
			CoreKitBasicUIBuild::AddH(NameRow, CoreKitBasicUIBuild::Text(WidgetTree, FText::FromString(FString::Printf(TEXT("×%d"), Card.Copies)), M(11.f, 500, CoreKitBasicUIColor::Steel, 0.f)));
		}
		CoreKitBasicUIBuild::AddH(Row, NameRow, FMargin(0.f, 0.f, 12.f, 0.f), VAlign_Center, HAlign_Left, 1.f);
		CoreKitBasicUIBuild::AddH(Row, CoreKitBasicUIBuild::Text(WidgetTree, Card.SummedEffect, D(18.f, 600, CoreKitBasicUIHudLogic::SummedEffectColor(Card))));
		if (Index + 1 < InCards.Num())
		{
			CoreKitBasicUIBuild::AddO(RowStack, CoreKitBasicUIBuild::Box(WidgetTree, Solid(WidgetTree, CoreKitBasicUIColor::WhiteA(0.08f)), 0.f, 1.f), HAlign_Fill, VAlign_Bottom);
		}
	}
	LastCards = InCards;
}

void UCoreKitBasicUIHudBuildInspector::TickMotion(float Dt)
{
	const UCoreKitBasicUIContextSubsystem* Ctx = GetUIContext();
	const UCoreKitBasicUIBuildViewModel* Build = Ctx ? Ctx->GetBuild() : nullptr;
	const UCoreKitBasicUIProgressionViewModel* Progression = Ctx ? Ctx->GetProgression() : nullptr;
	if (!Build || !Progression)
	{
		return;
	}
	if (!SameCards(Build->Cards, LastCards))
	{
		RebuildRows(Build->Cards);
	}
	if (Build->TotalCopies != LastCopies)
	{
		LastCopies = Build->TotalCopies;
		Header->SetText(FText::Format(COREKIT_BASICUI_TEXT("BuildCount", "BUILD · {0} CARDS"), FText::AsNumber(LastCopies)));
	}
	SetTextIfChanged(Archetype, Build->Archetype.ToUpper());

	// Hold TAB: rises (SETTLE), the game keeps running; release closes it (EXIT).
	const bool bWant = Build->bInspecting && !Progression->bOfferOpen;
	if (bWant != bOpen)
	{
		bOpen = bWant;
		Transition.Start(bOpen ? Token(ECoreKitBasicUIMotion::Settle) : Token(ECoreKitBasicUIMotion::Exit));
		if (bOpen)
		{
			Show(Panel, true);
		}
	}
	if (Transition.Tick(Dt))
	{
		const float A = Transition.Alpha();
		const float Visible = bOpen ? Ease(ECoreKitBasicUIMotion::Settle, A) : 1.f - Ease(ECoreKitBasicUIMotion::Exit, A);
		SetRenderOpacity(Visible);
		SetXform(Panel, FVector2D(0.f, 12.f * (1.f - Visible) * MotionAmount()));
		if (!bOpen && !Transition.IsRunning())
		{
			Show(Panel, false);
		}
	}
}

#undef LOCTEXT_NAMESPACE
