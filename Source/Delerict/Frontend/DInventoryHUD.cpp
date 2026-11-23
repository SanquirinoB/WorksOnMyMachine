#include "DInventoryHUD.h"

#include "DMenuStyle.h"
#include "../Player/DPawn.h"
#include "../Player/DPlayerTuning.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SafeZone.h"
#include "Components/SafeZoneSlot.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "DInventoryHUD"

TSharedRef<SWidget> UDInventoryHUD::RebuildWidget()
{
    if (!Items)
    {
        auto* Safe = WidgetTree->ConstructWidget<USafeZone>();
        WidgetTree->RootWidget = Safe;
        Items = WidgetTree->ConstructWidget<UWrapBox>();
        Items->SetInnerSlotPadding(FVector2D(5.f, 5.f));
        auto* Placement = CastChecked<USafeZoneSlot>(Safe->AddChild(Items));
        Placement->SetHorizontalAlignment(HAlign_Fill);
        Placement->SetVerticalAlignment(VAlign_Bottom);
        Placement->SetPadding(FMargin(24.f, 0.f, 24.f, 20.f));
    }
    return Super::RebuildWidget();
}

void UDInventoryHUD::BuildSlots(const ADPawn& Pawn)
{
    using namespace DelerictUI;
    Items->ClearChildren();
    ItemSlots.Reset();
    const UDPlayerTuning& Tuning = Pawn.GetTuning();
    TArray<FName> Types;
    Tuning.ThrowableMassByType.GenerateKeyArray(Types);
    for (const auto& Binding : Tuning.ThrowableHotkeys)
    {
        if (Binding.Value.IsValid()) { Types.AddUnique(Binding.Key); }
    }
    Types.AddUnique(Tuning.StartingPlateType);
    Types.Remove(NAME_None);
    Types.Sort([&Tuning](FName A, FName B)
    {
        const FKey KeyA = Tuning.ThrowableHotkeys.FindRef(A);
        const FKey KeyB = Tuning.ThrowableHotkeys.FindRef(B);
        if (KeyA.IsValid() != KeyB.IsValid()) { return KeyA.IsValid(); }
        if (KeyA.IsValid() && KeyB.IsValid())
        {
            const int32 Order = KeyA.GetDisplayName(false).CompareTo(KeyB.GetDisplayName(false));
            if (Order != 0) { return Order < 0; }
        }
        return A.LexicalLess(B);
    });

    for (FName Type : Types)
    {
        auto* Card = Surface(*WidgetTree, Panel, FMargin(10.f, 5.f), 1.f);
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Card->SetContent(Row);
        const FKey Key = Tuning.ThrowableHotkeys.FindRef(Type);
        if (Key.IsValid())
        {
            auto* Keycap = Surface(*WidgetTree, Field, FMargin(5.f, 0.f), 1.f);
            Keycap->SetContent(Label(*WidgetTree, Key.GetDisplayName(false), 13, Family::Mono, 600));
            auto* KeySlot = Row->AddChildToHorizontalBox(Keycap);
            KeySlot->SetVerticalAlignment(VAlign_Center);
            KeySlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
        }
        const auto Name = FText::FromString(FName::NameToDisplayString(Type.ToString(), false));
        Row->AddChildToHorizontalBox(Label(*WidgetTree, Name, 17, Family::Display, 600))->SetVerticalAlignment(VAlign_Center);
        auto* Quantity = Label(*WidgetTree, FText::GetEmpty(), 14, Family::Mono, 600);
        auto* CountSlot = Row->AddChildToHorizontalBox(Quantity);
        CountSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
        CountSlot->SetVerticalAlignment(VAlign_Center);
        Items->AddChildToWrapBox(Card);
        FItemSlot& Entry = ItemSlots.AddDefaulted_GetRef();
        Entry.Type = Type;
        Entry.Card = Card;
        Entry.Quantity = Quantity;
        Entry.bHasHotkey = Key.IsValid();
    }
}

void UDInventoryHUD::Refresh(const ADPawn& Pawn)
{
    if (!Items) { return; }
    const double Now = Pawn.GetWorld()->GetRealTimeSeconds();
    if (DisplayedPawn.Get() != &Pawn)
    {
        DisplayedPawn = &Pawn;
        BuildSlots(Pawn);
    }
    else if (Now < NextRefresh) { return; }
    // Counts include live pickup actors, which can also disappear when a streamed room unloads.
    NextRefresh = Now + .1;
    const FName Selected = Pawn.GetSelectedThrowableType();
    const auto& Limits = Pawn.GetTuning().ThrowableCarryLimits;
    using namespace DelerictUI;
    for (FItemSlot& Entry : ItemSlots)
    {
        const int32 Count = Pawn.GetThrowableCountByType(Entry.Type);
        const bool bSelected = Entry.Type == Selected;
        if (Entry.Count == Count && Entry.bSelected == bSelected) { continue; }
        Entry.Count = Count;
        Entry.bSelected = bSelected;
        const bool bVisible = Entry.bHasHotkey || bSelected || Count > 0;
        Entry.Card->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        if (!bVisible) { continue; }
        Entry.Card->SetBrush(FSlateRoundedBoxBrush(Panel, 0.f, bSelected ? Accent : Edge, bSelected ? 2.f : 1.f));
        Entry.Card->SetRenderOpacity(Count > 0 || bSelected ? 1.f : .75f);
        Entry.Quantity->SetColorAndOpacity(bSelected ? Accent : Ink);
        const int32* Limit = Limits.Find(Entry.Type);
        Entry.Quantity->SetText(Limit
            ? FText::Format(LOCTEXT("LimitedCount", "{0}/{1}"), FText::AsNumber(Count), FText::AsNumber(*Limit))
            : FText::Format(LOCTEXT("Count", "×{0}"), FText::AsNumber(Count)));
    }
}

#undef LOCTEXT_NAMESPACE
