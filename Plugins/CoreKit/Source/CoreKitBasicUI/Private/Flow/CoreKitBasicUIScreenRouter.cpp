#include "CoreKitBasicUI/Flow/CoreKitBasicUIScreenRouter.h"

#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitNavigation/CoreKitMenuWidget.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "CoreKitBasicUI/Flow/CoreKitBasicUIHUD.h"
#include "GameFramework/PlayerController.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUIViewModels.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUICardDrawMenu.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIEndScreens.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIPauseMenu.h"

namespace
{
	bool IsOpen(const UCoreKitNavigationSubsystem* Nav, const UCoreKitMenuWidget* Menu)
	{
		return Nav && Menu && Nav->IsMenuOpen(Menu);
	}
}

void UCoreKitBasicUIScreenRouter::Initialize(ACoreKitBasicUIHUD* InHud, UCoreKitBasicUIContextSubsystem* InContext)
{
	Hud = InHud;
	Context = InContext;
	HudOpacity.Snap(1.f);
	HudTarget = 1.f;
	RequestedHud = 1.f;
	if (!TickHandle.IsValid())
	{
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UCoreKitBasicUIScreenRouter::Tick));
	}
}

void UCoreKitBasicUIScreenRouter::Shutdown()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	if (UCoreKitNavigationSubsystem* Nav = GetNavigation())
	{
		for (UCoreKitMenuWidget* Menu : TArray<UCoreKitMenuWidget*>{ CardDraw.Get(), Pause.Get(), EndScreen.Get() })
		{
			if (IsOpen(Nav, Menu))
			{
				Nav->PopMenu(Menu);
			}
		}
		Nav->PopAllMenus(TEXT("Modal"));
	}
	CardDraw.Reset();
	Pause.Reset();
	EndScreen.Reset();
	if (ACoreKitBasicUIHUD* HudActor = Hud.Get(); HudActor && HudActor->GetLayout())
	{
		HudActor->GetLayout()->SetRenderOpacity(1.f);
	}
}

ECoreKitBasicUIRouterAction UCoreKitBasicUIScreenRouter::Decide(const FCoreKitBasicUIRouterInputs& In)
{
	const bool bTerminal = In.Phase == ECoreKitBasicUIRunPhase::Victory || In.Phase == ECoreKitBasicUIRunPhase::Defeat;
	if (bTerminal)
	{
		const bool bVictory = In.Phase == ECoreKitBasicUIRunPhase::Victory;
		if (In.bEndOpen)
		{
			return In.bEndIsVictory == bVictory ? ECoreKitBasicUIRouterAction::None : ECoreKitBasicUIRouterAction::CloseEnd;
		}
		if (In.bCardDrawOpen || In.bPauseOpen || In.bModalOpen)
		{
			return ECoreKitBasicUIRouterAction::CloseRunMenus;
		}
		return bVictory ? ECoreKitBasicUIRouterAction::OpenVictory : ECoreKitBasicUIRouterAction::OpenDefeat;
	}
	if (In.bEndOpen)
	{
		// Retry in place: the run is live again.
		return ECoreKitBasicUIRouterAction::CloseEnd;
	}
	if (In.bModalOpen)
	{
		// Nothing new opens over a modal; it is decided first.
		return ECoreKitBasicUIRouterAction::None;
	}
	if (In.bOfferOpen && In.OfferCards > 0)
	{
		return In.bCardDrawOpen || In.bPauseOpen ? ECoreKitBasicUIRouterAction::None : ECoreKitBasicUIRouterAction::OpenCardDraw;
	}
	if (In.bCardDrawOpen)
	{
		// Card Draw is playing its pick / unfreeze and closes itself.
		return ECoreKitBasicUIRouterAction::None;
	}
	if (In.bPaused)
	{
		return !In.bPauseOpen && In.PauseHeldFrames >= 2 ? ECoreKitBasicUIRouterAction::OpenPause : ECoreKitBasicUIRouterAction::None;
	}
	if (In.bPauseOpen && !In.bPauseClosing)
	{
		return ECoreKitBasicUIRouterAction::ClosePause;
	}
	return ECoreKitBasicUIRouterAction::None;
}

UCoreKitNavigationSubsystem* UCoreKitBasicUIScreenRouter::GetNavigation() const
{
	const APlayerController* Controller = GetController();
	const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	return Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
}

APlayerController* UCoreKitBasicUIScreenRouter::GetController() const
{
	const ACoreKitBasicUIHUD* HudActor = Hud.Get();
	return HudActor ? HudActor->GetOwningPlayerController() : nullptr;
}

FCoreKitBasicUIRouterInputs UCoreKitBasicUIScreenRouter::GatherInputs() const
{
	FCoreKitBasicUIRouterInputs In;
	const UCoreKitBasicUIContextSubsystem* Ctx = Context.Get();
	const UCoreKitNavigationSubsystem* Nav = GetNavigation();
	if (const UCoreKitBasicUIRunViewModel* Run = Ctx ? Ctx->GetRun() : nullptr)
	{
		In.Phase = Run->GetPhase();
		In.bPaused = Run->GetbPaused();
	}
	if (const UCoreKitBasicUIProgressionViewModel* Progression = Ctx ? Ctx->GetProgression() : nullptr)
	{
		In.bOfferOpen = Progression->GetbOfferOpen();
		In.OfferCards = Progression->GetOffer().Num();
	}
	In.bCardDrawOpen = IsOpen(Nav, CardDraw.Get());
	In.bPauseOpen = IsOpen(Nav, Pause.Get());
	In.bPauseClosing = In.bPauseOpen && Pause->IsClosing();
	In.bEndOpen = IsOpen(Nav, EndScreen.Get());
	In.bEndIsVictory = bEndVictory;
	In.bModalOpen = Nav && Nav->GetTopMenuInLayer(TEXT("Modal")) != nullptr;
	In.PauseHeldFrames = PauseHeldFrames;
	return In;
}

bool UCoreKitBasicUIScreenRouter::Tick(float DeltaSeconds)
{
	if (!Hud.IsValid() || !GetController())
	{
		return true;
	}
	FCoreKitBasicUIRouterInputs In = GatherInputs();
	const bool bTerminal = In.Phase == ECoreKitBasicUIRunPhase::Victory || In.Phase == ECoreKitBasicUIRunPhase::Defeat;
	PauseHeldFrames = In.bPaused && !bTerminal && !In.bOfferOpen && !In.bCardDrawOpen ? PauseHeldFrames + 1 : 0;
	In.PauseHeldFrames = PauseHeldFrames;
	Apply(Decide(In), In);
	UpdateHudOpacity(FMath::Min(DeltaSeconds, 0.1f));
	return true;
}

void UCoreKitBasicUIScreenRouter::Apply(ECoreKitBasicUIRouterAction Action, const FCoreKitBasicUIRouterInputs& In)
{
	UCoreKitNavigationSubsystem* Nav = GetNavigation();
	APlayerController* Controller = GetController();
	if (!Nav || !Controller)
	{
		return;
	}
	switch (Action)
	{
	case ECoreKitBasicUIRouterAction::OpenCardDraw:
		if (UCoreKitBasicUICardDrawMenu* Menu = CreateWidget<UCoreKitBasicUICardDrawMenu>(Controller, UCoreKitBasicUICardDrawMenu::StaticClass()))
		{
			RequestedHud = 1.f;
			CardDraw = Menu;
			Nav->PushMenu(Menu, TEXT("Menu"));
		}
		break;
	case ECoreKitBasicUIRouterAction::OpenPause:
		if (UCoreKitBasicUIPauseMenu* Menu = CreateWidget<UCoreKitBasicUIPauseMenu>(Controller, UCoreKitBasicUIPauseMenu::StaticClass()))
		{
			Pause = Menu;
			Nav->PushMenu(Menu, TEXT("Menu"));
		}
		break;
	case ECoreKitBasicUIRouterAction::ClosePause:
		if (UCoreKitBasicUIPauseMenu* Menu = Pause.Get())
		{
			Menu->BeginExit();
		}
		break;
	case ECoreKitBasicUIRouterAction::OpenVictory:
	case ECoreKitBasicUIRouterAction::OpenDefeat:
	{
		const bool bVictory = Action == ECoreKitBasicUIRouterAction::OpenVictory;
		UClass* Class = bVictory ? UCoreKitBasicUIVictoryScreen::StaticClass() : UCoreKitBasicUIDefeatScreen::StaticClass();
		if (UCoreKitBasicUIEndScreen* Menu = CreateWidget<UCoreKitBasicUIEndScreen>(Controller, Class))
		{
			bEndVictory = bVictory;
			EndScreen = Menu;
			Nav->PushMenu(Menu, TEXT("Menu"));
		}
		break;
	}
	case ECoreKitBasicUIRouterAction::CloseEnd:
		if (UCoreKitBasicUIEndScreen* Menu = EndScreen.Get())
		{
			Menu->CloseMenu();
		}
		EndScreen.Reset();
		break;
	case ECoreKitBasicUIRouterAction::CloseRunMenus:
		// The run ended: drop prompts, Pause (and anything it opened) and Card Draw without exit animations.
		Nav->PopAllMenus(TEXT("Modal"));
		Nav->PopAllMenus(TEXT("Menu"));
		CardDraw.Reset();
		Pause.Reset();
		break;
	default:
		break;
	}
}

void UCoreKitBasicUIScreenRouter::SetHudOpacity(float Target, float Seconds)
{
	RequestedHud = FMath::Clamp(Target, 0.f, 1.f);
	if (const UCoreKitNavigationSubsystem* Nav = GetNavigation(); IsOpen(Nav, CardDraw.Get()) && !FMath::IsNearlyEqual(HudTarget, RequestedHud))
	{
		HudTarget = RequestedHud;
		HudOpacity.Start(HudTarget, ECoreKitBasicUIMotion::Settle, FMath::Max(0.f, Seconds));
	}
}

void UCoreKitBasicUIScreenRouter::UpdateHudOpacity(float DeltaSeconds)
{
	const UCoreKitNavigationSubsystem* Nav = GetNavigation();
	float Desired = 1.f;
	float Seconds = 0.25f;
	if (IsOpen(Nav, EndScreen.Get()))
	{
		Desired = 0.f;
	}
	else if (IsOpen(Nav, Pause.Get()) && !Pause->IsClosing())
	{
		Desired = 0.f;
		Seconds = 0.2f;
	}
	else if (IsOpen(Nav, CardDraw.Get()))
	{
		// The HUD dims its own blocks (0.25, hand 0.35) while the offer is open; only an extra request applies here.
		Desired = RequestedHud;
	}
	else
	{
		RequestedHud = 1.f;
	}
	if (!FMath::IsNearlyEqual(Desired, HudTarget))
	{
		HudTarget = Desired;
		HudOpacity.Start(Desired, ECoreKitBasicUIMotion::Settle, Seconds);
	}
	HudOpacity.Tick(DeltaSeconds);
	if (ACoreKitBasicUIHUD* HudActor = Hud.Get(); HudActor && HudActor->GetLayout())
	{
		const float Value = FMath::Clamp(HudOpacity.Value, 0.f, 1.f);
		if (!FMath::IsNearlyEqual(HudActor->GetLayout()->GetRenderOpacity(), Value, 0.001f))
		{
			HudActor->GetLayout()->SetRenderOpacity(Value);
		}
	}
}

bool UCoreKitBasicUIScreenRouter::IsModalOpen() const
{
	const UCoreKitNavigationSubsystem* Nav = GetNavigation();
	return Nav && Nav->GetTopMenuInLayer(TEXT("Modal")) != nullptr;
}

bool UCoreKitBasicUIScreenRouter::IsOverlayActive() const
{
	const UCoreKitNavigationSubsystem* Nav = GetNavigation();
	return IsOpen(Nav, CardDraw.Get()) || IsOpen(Nav, Pause.Get()) || IsOpen(Nav, EndScreen.Get()) || IsModalOpen();
}
