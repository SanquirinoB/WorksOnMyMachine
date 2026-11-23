#include "CoreKitNavigation/CoreKitActionGlyphWidget.h"

#include "CoreKitNavigation/CoreKitNavigationGlyphs.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

void UCoreKitActionGlyphWidget::SetGlyphSource(UInputAction* InAction, UCoreKitNavigationGlyphs* InGlyphs)
{
	Action = InAction;
	Glyphs = InGlyphs;
	BindPlayer();
	RefreshGlyph();
}

void UCoreKitActionGlyphWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (!IsDesignTime())
	{
		BindPlayer();
		RefreshGlyph();
	}
}

void UCoreKitActionGlyphWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	UnbindPlayer();
	Super::ReleaseSlateResources(bReleaseChildren);
}

void UCoreKitActionGlyphWidget::BindPlayer()
{
	UnbindPlayer();
	if (ULocalPlayer* Player = GetOwningLocalPlayer())
	{
		Navigation = Player->GetSubsystem<UCoreKitNavigationSubsystem>();
		EnhancedInput = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (auto* Nav = Navigation.Get())
		{
			Nav->OnInputDeviceChanged.AddUniqueDynamic(this, &ThisClass::HandleDeviceChanged);
			Nav->OnGamepadTypeChanged.AddUniqueDynamic(this, &ThisClass::HandleGamepadTypeChanged);
		}
		if (auto* Input = EnhancedInput.Get())
		{
			Input->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &ThisClass::RefreshGlyph);
		}
	}
}

void UCoreKitActionGlyphWidget::UnbindPlayer()
{
	if (auto* Nav = Navigation.Get())
	{
		Nav->OnInputDeviceChanged.RemoveDynamic(this, &ThisClass::HandleDeviceChanged);
		Nav->OnGamepadTypeChanged.RemoveDynamic(this, &ThisClass::HandleGamepadTypeChanged);
	}
	if (auto* Input = EnhancedInput.Get())
	{
		Input->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &ThisClass::RefreshGlyph);
	}
	Navigation.Reset();
	EnhancedInput.Reset();
}

void UCoreKitActionGlyphWidget::HandleDeviceChanged(ECoreKitInputDevice)
{
	RefreshGlyph();
}

void UCoreKitActionGlyphWidget::HandleGamepadTypeChanged(FName)
{
	RefreshGlyph();
}

void UCoreKitActionGlyphWidget::RefreshGlyph()
{
	UCoreKitNavigationSubsystem* Nav = Navigation.Get();
	if (!Nav)
	{
		SetBrushFromTexture(nullptr, false);
		return;
	}
	if (Glyphs)
	{
		SetBrushFromTexture(Nav->FindActionGlyph(Action, Glyphs), false);
		return;
	}
	// Without an explicit glyph asset, use the project's icon sets for the current device and gamepad type.
	FKey Key;
	FCoreKitKeyPresentation Presentation;
	Nav->FindActionPresentation(Action, Key, Presentation);
	SetBrushFromTexture(Presentation.Icon.LoadSynchronous(), false);
}
