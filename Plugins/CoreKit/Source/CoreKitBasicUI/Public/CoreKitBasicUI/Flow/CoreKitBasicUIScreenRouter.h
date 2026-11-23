#pragma once

#include "Containers/Ticker.h"
#include "UObject/Object.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUITypes.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUIScreenRouter.generated.h"

class ACoreKitBasicUIHUD;
class APlayerController;
class UCoreKitMenuWidget;
class UCoreKitNavigationSubsystem;
class UCoreKitBasicUIContextSubsystem;
class UCoreKitBasicUICardDrawMenu;
class UCoreKitBasicUIPauseMenu;
class UCoreKitBasicUIEndScreen;

/** Snapshot of the UI state the router reacts to (pure data, for the decision function and its tests). */
struct COREKITBASICUI_API FCoreKitBasicUIRouterInputs
{
	ECoreKitBasicUIRunPhase Phase = ECoreKitBasicUIRunPhase::Survival;
	bool bPaused = false;
	bool bOfferOpen = false;
	int32 OfferCards = 0;
	bool bCardDrawOpen = false;
	bool bPauseOpen = false;
	bool bPauseClosing = false;
	/** Victory or Defeat screen on the stack. */
	bool bEndOpen = false;
	bool bEndIsVictory = false;
	/** Any menu on the Modal layer (confirmation prompt). */
	bool bModalOpen = false;
	/** Frames the pause condition has held (debounce of one frame against intermediate VM states). */
	int32 PauseHeldFrames = 0;
};

enum class ECoreKitBasicUIRouterAction : uint8
{
	None,
	OpenCardDraw,
	OpenPause,
	ClosePause,
	OpenVictory,
	OpenDefeat,
	/** Terminal screen of the other kind is open, or the run left the terminal phase (retry in place). */
	CloseEnd,
	/** The run ended: close Card Draw / Pause / prompts before opening the end screen. */
	CloseRunMenus
};

/**
 * Turns UI state (Run phase, pause, card offers) into menus on the CoreKitNavigation stack:
 * Card Draw and Pause on "Menu", confirmations on "Modal", Victory/Defeat on "Menu". Queues HUD banners while a
 * modal is open. Owned by ACoreKitBasicUIHUD.
 *
 * Polls the ViewModels on the core ticker (it runs while the game is paused). Never opens anything over a modal,
 * never stacks a second Card Draw (chained offers re-deal inside the open one), and restores the HUD opacity once no
 * overlay is open.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIScreenRouter : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(ACoreKitBasicUIHUD* InHud, UCoreKitBasicUIContextSubsystem* InContext);
	virtual void Shutdown();

	/** Pure decision used by Tick (tested by CoreKitBasicUI.UI.Overlays.RouterDecisions). */
	static ECoreKitBasicUIRouterAction Decide(const FCoreKitBasicUIRouterInputs& Inputs);

	/** HUD layer opacity requested by an overlay (Card Draw dims to 0.25); the router restores 1 after it closes. */
	void SetHudOpacity(float Target, float Seconds);

	/** True while Card Draw, Pause, an end screen or a modal is open (HUD banners wait; "one hero at a time"). */
	bool IsOverlayActive() const;
	bool IsModalOpen() const;

	UCoreKitBasicUICardDrawMenu* GetCardDraw() const { return CardDraw.Get(); }
	UCoreKitBasicUIPauseMenu* GetPause() const { return Pause.Get(); }
	UCoreKitBasicUIEndScreen* GetEndScreen() const { return EndScreen.Get(); }

protected:
	ACoreKitBasicUIHUD* GetHud() const { return Hud.Get(); }
	UCoreKitBasicUIContextSubsystem* GetContext() const { return Context.Get(); }

	bool Tick(float DeltaSeconds);
	FCoreKitBasicUIRouterInputs GatherInputs() const;
	UCoreKitNavigationSubsystem* GetNavigation() const;
	APlayerController* GetController() const;
	void Apply(ECoreKitBasicUIRouterAction Action, const FCoreKitBasicUIRouterInputs& Inputs);
	void UpdateHudOpacity(float DeltaSeconds);

private:
	TWeakObjectPtr<ACoreKitBasicUIHUD> Hud;
	TWeakObjectPtr<UCoreKitBasicUIContextSubsystem> Context;

	TWeakObjectPtr<UCoreKitBasicUICardDrawMenu> CardDraw;
	TWeakObjectPtr<UCoreKitBasicUIPauseMenu> Pause;
	TWeakObjectPtr<UCoreKitBasicUIEndScreen> EndScreen;
	FTSTicker::FDelegateHandle TickHandle;
	FCoreKitBasicUITween HudOpacity;
	float RequestedHud = 1.f;
	float HudTarget = 1.f;
	int32 PauseHeldFrames = 0;
	bool bEndVictory = false;
};
