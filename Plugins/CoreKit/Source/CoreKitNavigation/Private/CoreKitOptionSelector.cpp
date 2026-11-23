#include "CoreKitNavigation/CoreKitOptionSelector.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "CoreKitNavigation/CoreKitButton.h"

TSharedRef<SWidget> UCoreKitOptionSelector::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OptionPanel"));
		WidgetTree->RootWidget = Row;
		OptionPanel = Row;
	}
	return Super::RebuildWidget();
}

void UCoreKitOptionSelector::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildButtons();
}

void UCoreKitOptionSelector::SetOptions(const TArray<FText>& InOptions, int32 InSelected)
{
	Options = InOptions;
	SelectedIndex = Options.IsEmpty() ? 0 : FMath::Clamp(InSelected, 0, Options.Num() - 1);
	RebuildButtons();
}

UCoreKitButton* UCoreKitOptionSelector::GetOptionButton(int32 Index) const
{
	return Buttons.IsValidIndex(Index) ? Buttons[Index].Get() : nullptr;
}

void UCoreKitOptionSelector::SetSelectedIndex(int32 Index, bool bNotify)
{
	if (Options.IsEmpty())
	{
		return;
	}
	const int32 Clamped = FMath::Clamp(Index, 0, Options.Num() - 1);
	const bool bChanged = Clamped != SelectedIndex;
	SelectedIndex = Clamped;
	RefreshVisuals();
	if (bChanged && bNotify)
	{
		OnValueChanged.Broadcast(SelectedIndex);
	}
}

void UCoreKitOptionSelector::Step(int32 Delta)
{
	const int32 Count = Options.Num();
	if (Count == 0 || Delta == 0)
	{
		return;
	}
	int32 Next = SelectedIndex + Delta;
	if (bWrap && Mode == ECoreKitOptionSelectorMode::Arrows)
	{
		Next = (Next % Count + Count) % Count;
	}
	const bool bWasSelectedFocused = Mode == ECoreKitOptionSelectorMode::Segmented && Buttons.IsValidIndex(SelectedIndex) && Buttons[SelectedIndex]->HasKeyboardFocus();
	SetSelectedIndex(Next);
	if (bWasSelectedFocused && Buttons.IsValidIndex(SelectedIndex))
	{
		Buttons[SelectedIndex]->Select();
	}
}

void UCoreKitOptionSelector::FocusSelector()
{
	if (Mode == ECoreKitOptionSelectorMode::Segmented && Buttons.IsValidIndex(SelectedIndex))
	{
		Buttons[SelectedIndex]->Select();
	}
	else if (Buttons.Num() > 0)
	{
		Buttons.Last()->Select();
	}
}

FNavigationReply UCoreKitOptionSelector::NativeOnNavigation(const FGeometry& Geometry, const FNavigationEvent& Event, const FNavigationReply& Default)
{
	const EUINavigation Direction = Event.GetNavigationType();
	if (Direction == EUINavigation::Left || Direction == EUINavigation::Right)
	{
		Step(Direction == EUINavigation::Left ? -1 : 1);
		return FNavigationReply::Stop();
	}
	return Super::NativeOnNavigation(Geometry, Event, Default);
}

void UCoreKitOptionSelector::RebuildButtons()
{
	if (!OptionPanel || !WidgetTree)
	{
		return;
	}
	OptionPanel->ClearChildren();
	Buttons.Reset();
	auto MakeButton = [this](const FText& Text, int32 Index)
	{
		UCoreKitButton* Button = WidgetTree->ConstructWidget<UCoreKitButton>(UCoreKitButton::StaticClass());
		Button->ButtonIndex = Index;
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Text);
		Button->AddChild(Label);
		Button->OnActivated.AddDynamic(this, &ThisClass::HandleButtonActivated);
		return Button;
	};
	if (Mode == ECoreKitOptionSelectorMode::Segmented)
	{
		for (int32 Index = 0; Index < Options.Num(); ++Index)
		{
			UCoreKitButton* Button = MakeButton(Options[Index], Index);
			if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(OptionPanel->AddChild(Button)))
			{
				BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
			Buttons.Add(Button);
		}
	}
	else
	{
		UCoreKitButton* Prev = MakeButton(FText::FromString(TEXT("<")), -1);
		UCoreKitButton* Next = MakeButton(FText::FromString(TEXT(">")), -2);
		if (!ValueLabel)
		{
			ValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValueLabel"));
		}
		OptionPanel->AddChild(Prev);
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(OptionPanel->AddChild(ValueLabel)))
		{
			BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			BoxSlot->SetHorizontalAlignment(HAlign_Center);
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		OptionPanel->AddChild(Next);
		Buttons.Add(Prev);
		Buttons.Add(Next);
	}
	RefreshVisuals();
}

void UCoreKitOptionSelector::RefreshVisuals()
{
	if (Mode == ECoreKitOptionSelectorMode::Arrows)
	{
		if (ValueLabel && Options.IsValidIndex(SelectedIndex))
		{
			ValueLabel->SetText(Options[SelectedIndex]);
		}
		for (UCoreKitButton* Button : Buttons)
		{
			OnStyleOption.Broadcast(Button, Button->ButtonIndex, false);
		}
		return;
	}
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		OnStyleOption.Broadcast(Buttons[Index], Index, Index == SelectedIndex);
	}
}

void UCoreKitOptionSelector::HandleButtonActivated(UCoreKitButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button->ButtonIndex == -1)
	{
		Step(-1);
	}
	else if (Button->ButtonIndex == -2)
	{
		Step(1);
	}
	else
	{
		SetSelectedIndex(Button->ButtonIndex);
	}
}
