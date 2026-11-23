#include "CoreKitNavigation/CoreKitPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitNavigation/CoreKitButton.h"

TSharedRef<SWidget> UCoreKitPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PromptRoot"));
		WidgetTree->RootWidget = Column;
		Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		Body = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
		Body->SetAutoWrapText(true);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OptionPanel"));
		OptionPanel = Row;
		Column->AddChild(Title);
		Column->AddChild(Body);
		Column->AddChild(Row);
	}
	return Super::RebuildWidget();
}

void UCoreKitPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildOptions();
}

void UCoreKitPromptWidget::SetupPrompt(const FText& InTitle, const FText& InBody, const TArray<FText>& InOptions, int32 InCancelOption, int32 InDefaultOption)
{
	PromptTitle = InTitle;
	PromptBody = InBody;
	Options = InOptions;
	CancelOption = FMath::Clamp(InCancelOption, 0, FMath::Max(0, Options.Num() - 1));
	DefaultOption = FMath::Clamp(InDefaultOption, 0, FMath::Max(0, Options.Num() - 1));
	bDecided = false;
	RebuildOptions();
}

void UCoreKitPromptWidget::RebuildOptions()
{
	if (Title)
	{
		Title->SetText(PromptTitle);
	}
	if (Body)
	{
		Body->SetText(PromptBody);
		Body->SetVisibility(PromptBody.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (!OptionPanel || !WidgetTree)
	{
		return;
	}
	OptionPanel->ClearChildren();
	OptionButtons.Reset();
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		UCoreKitButton* Button = WidgetTree->ConstructWidget<UCoreKitButton>(UCoreKitButton::StaticClass());
		Button->ButtonIndex = Index;
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Options[Index]);
		Button->AddChild(Label);
		Button->OnActivated.AddDynamic(this, &ThisClass::HandleOptionActivated);
		OptionPanel->AddChild(Button);
		OptionButtons.Add(Button);
		OnStyleOption.Broadcast(Button, Index, Index == CancelOption);
	}
	NativeOnPromptSetup();
}

UWidget* UCoreKitPromptWidget::GetInitialFocus_Implementation() const
{
	if (OptionButtons.IsValidIndex(DefaultOption))
	{
		return OptionButtons[DefaultOption];
	}
	return Super::GetInitialFocus_Implementation();
}

bool UCoreKitPromptWidget::HandleReturn_Implementation()
{
	if (!bAllowReturn)
	{
		return false;
	}
	Decide(CancelOption);
	return true;
}

void UCoreKitPromptWidget::HandleOptionActivated(UCoreKitButton* Button)
{
	if (Button)
	{
		Decide(Button->ButtonIndex);
	}
}

void UCoreKitPromptWidget::Decide(int32 Option)
{
	if (bDecided)
	{
		return;
	}
	bDecided = true;
	const FCoreKitPromptCallback Pending = Callback;
	Callback.Unbind();
	CloseMenu();
	Pending.ExecuteIfBound(Option);
	OnPromptDecided.Broadcast(Option);
}
