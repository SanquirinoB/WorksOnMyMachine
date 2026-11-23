#include "CoreKitNavigation/CoreKitSectionList.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "CoreKitNavigation/CoreKitButton.h"

TSharedRef<SWidget> UCoreKitSectionList::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SectionPanel"));
		WidgetTree->RootWidget = Row;
		SectionPanel = Row;
	}
	return Super::RebuildWidget();
}

void UCoreKitSectionList::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildButtons();
}

void UCoreKitSectionList::SetSections(const TArray<FText>& InSections)
{
	Sections = InSections;
	ActiveSection = Sections.IsEmpty() ? 0 : FMath::Clamp(ActiveSection, 0, Sections.Num() - 1);
	RebuildButtons();
}

void UCoreKitSectionList::SetSwitcher(UWidgetSwitcher* InSwitcher)
{
	Switcher = InSwitcher;
	RefreshVisuals();
}

UCoreKitButton* UCoreKitSectionList::GetSectionButton(int32 Index) const
{
	return Buttons.IsValidIndex(Index) ? Buttons[Index].Get() : nullptr;
}

void UCoreKitSectionList::SetActiveSection(int32 Index)
{
	if (Sections.IsEmpty())
	{
		return;
	}
	const int32 Clamped = FMath::Clamp(Index, 0, Sections.Num() - 1);
	const bool bChanged = Clamped != ActiveSection;
	ActiveSection = Clamped;
	RefreshVisuals();
	if (bChanged)
	{
		OnSectionChanged.Broadcast(ActiveSection);
	}
}

void UCoreKitSectionList::StepSection(int32 Delta)
{
	const int32 Count = Sections.Num();
	if (Count == 0 || Delta == 0)
	{
		return;
	}
	SetActiveSection(((ActiveSection + Delta) % Count + Count) % Count);
}

void UCoreKitSectionList::RebuildButtons()
{
	if (!SectionPanel || !WidgetTree)
	{
		return;
	}
	Buttons.Reset();
	SectionPanel->ClearChildren();
	for (int32 Index = 0; Index < Sections.Num(); ++Index)
	{
		UCoreKitButton* Button = WidgetTree->ConstructWidget<UCoreKitButton>(UCoreKitButton::StaticClass());
		Button->ButtonIndex = Index;
		// Tabs change with Q/E or a click; they do not take focus from the page content.
		Button->SetFocusableBeforeConstruct(false);
		Button->bSelectOnHover = false;
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Sections[Index]);
		Button->AddChild(Label);
		Button->OnActivated.AddDynamic(this, &ThisClass::HandleButtonActivated);
		SectionPanel->AddChild(Button);
		Buttons.Add(Button);
	}
	RefreshVisuals();
}

void UCoreKitSectionList::RefreshVisuals()
{
	if (UWidgetSwitcher* Target = Switcher.Get(); Target && Target->GetNumWidgets() > ActiveSection)
	{
		Target->SetActiveWidgetIndex(ActiveSection);
	}
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		OnStyleTab.Broadcast(Buttons[Index], Index, Index == ActiveSection);
	}
}

void UCoreKitSectionList::HandleButtonActivated(UCoreKitButton* Button)
{
	if (Button)
	{
		SetActiveSection(Button->ButtonIndex);
	}
}
