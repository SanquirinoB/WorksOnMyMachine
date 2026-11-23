#include "CoreKitNavigation/CoreKitMenuWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "CoreKitNavigation/CoreKitNavigationSettings.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitNavigation/CoreKitSectionList.h"
#include "HAL/PlatformTime.h"

void UCoreKitMenuWidget::CloseMenu()
{
	if (UCoreKitNavigationSubsystem* Nav = Owner.Get())
	{
		Nav->PopMenu(this);
	}
	else
	{
		RemoveFromParent();
	}
}

bool UCoreKitMenuWidget::IsInputCoolingDown() const
{
	const float Cooldown = ActivationInputCooldown >= 0.f ? ActivationInputCooldown
		: GetDefault<UCoreKitNavigationSettings>()->DefaultActivationCooldown;
	return bActiveMenu && FPlatformTime::Seconds() - ActivatedAt < Cooldown;
}

UWidget* UCoreKitMenuWidget::GetInitialFocus_Implementation() const
{
	UWidget* Found = nullptr;
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([&Found](UWidget* Widget)
		{
			if (!Found && Widget && Widget->IsVisible() && Widget->GetIsEnabled() && Widget->TakeWidget()->SupportsKeyboardFocus())
			{
				Found = Widget;
			}
		});
	}
	return Found;
}

bool UCoreKitMenuWidget::HandleReturn_Implementation()
{
	if (!bAllowReturn)
	{
		return false;
	}
	CloseMenu();
	return true;
}

bool UCoreKitMenuWidget::HandleSection_Implementation(int32 Delta)
{
	if (!bAllowSections)
	{
		return false;
	}
	if (UCoreKitSectionList* List = FindSectionList())
	{
		List->StepSection(Delta);
		return true;
	}
	return false;
}

bool UCoreKitMenuWidget::HandleMenuKey_Implementation(FKey)
{
	return false;
}

void UCoreKitMenuWidget::SetSectionList(UCoreKitSectionList* InSections)
{
	Sections = InSections;
}

UCoreKitSectionList* UCoreKitMenuWidget::FindSectionList() const
{
	if (UCoreKitSectionList* List = Sections.Get())
	{
		return List;
	}
	UCoreKitSectionList* Found = nullptr;
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([&Found](UWidget* Widget)
		{
			if (!Found)
			{
				Found = Cast<UCoreKitSectionList>(Widget);
			}
		});
	}
	return Found;
}

void UCoreKitMenuWidget::NativeOnMenuActivated()
{
	OnMenuActivated();
}

void UCoreKitMenuWidget::NativeOnMenuDeactivated()
{
	OnMenuDeactivated();
}

void UCoreKitMenuWidget::SetMenuActive(bool bActive)
{
	if (bActive == bActiveMenu)
	{
		return;
	}
	bActiveMenu = bActive;
	if (bActive)
	{
		ActivatedAt = FPlatformTime::Seconds();
		NativeOnMenuActivated();
	}
	else
	{
		NativeOnMenuDeactivated();
	}
}

void UCoreKitMenuWidget::BeginRemoval()
{
	if (bRemoving)
	{
		return;
	}
	bRemoving = true;
	if (Anim_Out && IsInViewport())
	{
		FWidgetAnimationDynamicEvent Finished;
		Finished.BindDynamic(this, &ThisClass::HandleOutFinished);
		BindToAnimationFinished(Anim_Out, Finished);
		PlayAnimation(Anim_Out);
		return;
	}
	HandleOutFinished();
}

void UCoreKitMenuWidget::HandleOutFinished()
{
	if (Anim_Out)
	{
		UnbindAllFromAnimationFinished(Anim_Out);
	}
	bRemoving = false;
	RemoveFromParent();
}
