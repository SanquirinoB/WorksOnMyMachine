#include "CoreKitNavigation/CoreKitInputPrompt.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

TSharedRef<SWidget> UCoreKitInputPrompt::RebuildWidget()
{
	// A plain instance (no Designer layout) gets a default icon + label row.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		WidgetTree->RootWidget = Row;
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UCoreKitInputPrompt::NativeConstruct()
{
	Super::NativeConstruct();
	Bind();
	Refresh();
}

void UCoreKitInputPrompt::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UCoreKitInputPrompt::SetAction(UInputAction* InAction)
{
	Action = InAction;
	Refresh();
}

void UCoreKitInputPrompt::SetKey(FKey InKey)
{
	Key = InKey;
	Refresh();
}

void UCoreKitInputPrompt::Bind()
{
	Unbind();
	if (ULocalPlayer* Player = GetOwningLocalPlayer())
	{
		Navigation = Player->GetSubsystem<UCoreKitNavigationSubsystem>();
		EnhancedInput = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (UCoreKitNavigationSubsystem* Nav = Navigation.Get())
		{
			Nav->OnInputDeviceChanged.AddUniqueDynamic(this, &ThisClass::HandleDeviceChanged);
			Nav->OnGamepadTypeChanged.AddUniqueDynamic(this, &ThisClass::HandleGamepadTypeChanged);
		}
		if (UEnhancedInputLocalPlayerSubsystem* Input = EnhancedInput.Get())
		{
			Input->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &ThisClass::HandleMappingsRebuilt);
		}
	}
}

void UCoreKitInputPrompt::Unbind()
{
	if (UCoreKitNavigationSubsystem* Nav = Navigation.Get())
	{
		Nav->OnInputDeviceChanged.RemoveDynamic(this, &ThisClass::HandleDeviceChanged);
		Nav->OnGamepadTypeChanged.RemoveDynamic(this, &ThisClass::HandleGamepadTypeChanged);
	}
	if (UEnhancedInputLocalPlayerSubsystem* Input = EnhancedInput.Get())
	{
		Input->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &ThisClass::HandleMappingsRebuilt);
	}
	Navigation.Reset();
	EnhancedInput.Reset();
}

void UCoreKitInputPrompt::HandleDeviceChanged(ECoreKitInputDevice)
{
	Refresh();
}

void UCoreKitInputPrompt::HandleGamepadTypeChanged(FName)
{
	Refresh();
}

void UCoreKitInputPrompt::HandleMappingsRebuilt()
{
	Refresh();
}

void UCoreKitInputPrompt::Refresh()
{
	if (!Navigation.IsValid())
	{
		if (ULocalPlayer* Player = GetOwningLocalPlayer())
		{
			Navigation = Player->GetSubsystem<UCoreKitNavigationSubsystem>();
		}
	}

	DisplayedKey = FKey();
	Presentation = FCoreKitKeyPresentation();
	bool bHasKey = false;
	if (const UCoreKitNavigationSubsystem* Nav = Navigation.Get())
	{
		if (Key.IsValid())
		{
			DisplayedKey = Key;
			Presentation = Nav->FindKeyPresentation(Key);
			bHasKey = true;
		}
		else
		{
			bHasKey = Nav->FindActionPresentation(Action, DisplayedKey, Presentation);
		}
	}

	UTexture2D* Texture = bHasKey ? Presentation.Icon.LoadSynchronous() : nullptr;
	if (Icon)
	{
		Icon->SetBrushFromTexture(Texture, false);
		Icon->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Label)
	{
		const bool bShowLabel = bHasKey && (bAlwaysShowLabel || !Texture) && !Presentation.DisplayName.IsEmpty();
		Label->SetText(Presentation.DisplayName);
		Label->SetVisibility(bShowLabel ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	NativeOnPresentationChanged(Presentation, bHasKey);
	OnPresentationChanged(Presentation, bHasKey);
}
