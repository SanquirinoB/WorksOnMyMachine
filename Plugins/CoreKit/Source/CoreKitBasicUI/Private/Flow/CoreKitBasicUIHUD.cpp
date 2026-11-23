#include "CoreKitBasicUI/Flow/CoreKitBasicUIHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "CoreKitBasicUI/Flow/CoreKitBasicUIScreenRouter.h"
#include "GameFramework/PlayerController.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/HUD/CoreKitBasicUIHudRoot.h"

void UCoreKitBasicUIPrimaryLayout::BuildTree()
{
	ScreenCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("ScreenCanvas"));
	WidgetTree->RootWidget = ScreenCanvas;
	SafeCanvas = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("SafeCanvas"));
	// Title safe 90 %: 96 / 54 px at 1920x1080 (DPI scaling keeps the proportion at other resolutions).
	CoreKitBasicUIBuild::Fill(ScreenCanvas, SafeCanvas, FMargin(96.f, 54.f, 96.f, 54.f), 10);
	BuildHud();
}

void UCoreKitBasicUIPrimaryLayout::BuildHud()
{
	UCoreKitBasicUIHudRoot* Hud = WidgetTree->ConstructWidget<UCoreKitBasicUIHudRoot>(UCoreKitBasicUIHudRoot::StaticClass(), TEXT("HudRoot"));
	CoreKitBasicUIBuild::Fill(ScreenCanvas, Hud, FMargin(0.f), 5);
	Hud->Setup(ScreenCanvas, SafeCanvas);
}

ACoreKitBasicUIHUD::ACoreKitBasicUIHUD()
{
	LayoutClass = UCoreKitBasicUIPrimaryLayout::StaticClass();
}

void ACoreKitBasicUIHUD::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* Controller = GetOwningPlayerController();
	if (!Controller || !Controller->IsLocalController())
	{
		return;
	}
	UCoreKitBasicUIContextSubsystem* Context = Controller->GetLocalPlayer() ? Controller->GetLocalPlayer()->GetSubsystem<UCoreKitBasicUIContextSubsystem>() : nullptr;
	if (Context)
	{
		Context->StartRunPresentation(Controller);
	}
	Layout = CreateWidget<UCoreKitBasicUIPrimaryLayout>(Controller, LayoutClass ? LayoutClass.Get() : UCoreKitBasicUIPrimaryLayout::StaticClass());
	if (Layout)
	{
		Layout->AddToPlayerScreen(0);
	}
	Router = NewObject<UCoreKitBasicUIScreenRouter>(this);
	Router->Initialize(this, Context);
}

void ACoreKitBasicUIHUD::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Router)
	{
		Router->Shutdown();
	}
	if (Layout)
	{
		Layout->RemoveFromParent();
	}
	if (APlayerController* Controller = GetOwningPlayerController())
	{
		if (UCoreKitBasicUIContextSubsystem* Context = Controller->GetLocalPlayer() ? Controller->GetLocalPlayer()->GetSubsystem<UCoreKitBasicUIContextSubsystem>() : nullptr)
		{
			Context->StopRunPresentation();
		}
	}
	Super::EndPlay(Reason);
}

