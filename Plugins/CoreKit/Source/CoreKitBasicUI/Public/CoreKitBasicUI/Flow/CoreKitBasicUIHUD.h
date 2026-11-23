#pragma once

#include "GameFramework/HUD.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUIHUD.generated.h"

class UCanvasPanel;
class UCoreKitBasicUIScreenRouter;

/**
 * Root of the in-run HUD layer (safe zone 96/54 at 1920x1080). The HUD stage fills it with the HUD blocks.
 * Overlays (Card Draw, Pause, end screens) are menus pushed by UCoreKitBasicUIScreenRouter through CoreKitNavigation.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIPrimaryLayout : public UCoreKitBasicUIUserWidget
{
	GENERATED_BODY()

public:
	/** Full-screen canvas (world-attached widgets, vignettes). */
	UCanvasPanel* GetScreenCanvas() const { return ScreenCanvas; }
	/** Canvas inside the 90 % title-safe area. */
	UCanvasPanel* GetSafeCanvas() const { return SafeCanvas; }

protected:
	void BuildTree() override;
	/** HUD stage hook: add HUD blocks to the canvases. */
	virtual void BuildHud();

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ScreenCanvas;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SafeCanvas;
};

/** Game HUD actor for the run: creates the primary layout, starts the UI context and the screen router. */
UCLASS()
class COREKITBASICUI_API ACoreKitBasicUIHUD : public AHUD
{
	GENERATED_BODY()

public:
	ACoreKitBasicUIHUD();

	UCoreKitBasicUIPrimaryLayout* GetLayout() const { return Layout; }
	UCoreKitBasicUIScreenRouter* GetRouter() const { return Router; }

	UPROPERTY(EditDefaultsOnly, Category = "CoreKitBasicUI|UI")
	TSubclassOf<UCoreKitBasicUIPrimaryLayout> LayoutClass;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIPrimaryLayout> Layout;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIScreenRouter> Router;
};
