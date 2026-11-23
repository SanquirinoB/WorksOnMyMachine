#include "CoreKitNavigation/CoreKitButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ButtonSlot.h"
#include "CoreKitNavigation/CoreKitMenuWidget.h"
#include "CoreKitNavigation/CoreKitNavigationSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Widgets/Input/SButton.h"

TSharedRef<SWidget> UCoreKitButton::RebuildWidget()
{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (!bHasBaseStyle)
	{
		BaseStyle = WidgetStyle;
		bHasBaseStyle = true;
	}
	MyButton = SNew(SButton)
		.OnClicked_UObject(this, &ThisClass::HandleClicked)
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressed))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleased))
		.OnHovered_UObject(this, &ThisClass::HandleHovered)
		.OnUnhovered_UObject(this, &ThisClass::HandleUnhovered)
		.OnReceivedFocus_UObject(this, &ThisClass::HandleFocusReceived)
		.OnLostFocus_UObject(this, &ThisClass::HandleFocusLost)
		.ButtonStyle(&WidgetStyle)
		.ClickMethod(ClickMethod)
		.TouchMethod(TouchMethod)
		.PressMethod(PressMethod)
		.IsFocusable(IsFocusable);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}
	return MyButton.ToSharedRef();
}

void UCoreKitButton::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	bSelected = false;
}

void UCoreKitButton::Select()
{
	if (!MyButton.IsValid() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	if (ULocalPlayer* Player = GetOwningLocalPlayer())
	{
		if (const TSharedPtr<FSlateUser> User = Player->GetSlateUser())
		{
			FSlateApplication::Get().SetUserFocus(User->GetUserIndex(), MyButton);
			return;
		}
	}
	FSlateApplication::Get().SetKeyboardFocus(MyButton);
}

void UCoreKitButton::SetBaseStyle(const FButtonStyle& InStyle)
{
	BaseStyle = InStyle;
	bHasBaseStyle = true;
	ApplySelectedStyle();
}

UCoreKitMenuWidget* UCoreKitButton::FindOwningMenu() const
{
	for (UObject* Outer = GetOuter(); Outer; Outer = Outer->GetOuter())
	{
		if (UCoreKitMenuWidget* Menu = Cast<UCoreKitMenuWidget>(Outer))
		{
			return Menu;
		}
	}
	return nullptr;
}

FReply UCoreKitButton::HandleClicked()
{
	if (const UCoreKitMenuWidget* Menu = FindOwningMenu(); Menu && Menu->IsInputCoolingDown())
	{
		return FReply::Handled();
	}
	if (bPlaySounds)
	{
		if (USoundBase* Sound = GetDefault<UCoreKitNavigationSettings>()->ActivateSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Sound);
		}
	}
	OnClicked.Broadcast();
	OnActivated.Broadcast(this);
	return FReply::Handled();
}

void UCoreKitButton::HandleHovered()
{
	OnHovered.Broadcast();
	if (bSelectOnHover && !bSelected)
	{
		Select();
	}
}

void UCoreKitButton::HandleUnhovered()
{
	OnUnhovered.Broadcast();
}

void UCoreKitButton::HandleFocusReceived()
{
	SetSelected(true);
}

void UCoreKitButton::HandleFocusLost()
{
	SetSelected(false);
}

void UCoreKitButton::SetSelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}
	bSelected = bInSelected;
	ApplySelectedStyle();
	if (bSelected)
	{
		if (bPlaySounds)
		{
			if (USoundBase* Sound = GetDefault<UCoreKitNavigationSettings>()->NavigateSound.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(this, Sound);
			}
		}
		OnSelected.Broadcast(this);
	}
	else
	{
		OnDeselected.Broadcast(this);
	}
}

void UCoreKitButton::ApplySelectedStyle()
{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (!bHasBaseStyle)
	{
		BaseStyle = WidgetStyle;
		bHasBaseStyle = true;
	}
	WidgetStyle = BaseStyle;
	if (bSelected)
	{
		WidgetStyle.Normal = BaseStyle.Hovered;
	}
	if (MyButton.IsValid())
	{
		MyButton->SetButtonStyle(&WidgetStyle);
	}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
}
