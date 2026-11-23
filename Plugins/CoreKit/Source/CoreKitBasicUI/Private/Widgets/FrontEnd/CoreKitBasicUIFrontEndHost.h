#pragma once

#include "CoreKitFrontend/CoreKitFrontendMenu.h"
#include "CoreKitBasicUIFrontEndHost.generated.h"

class UCoreKitBasicUIMainMenu;

/**
 * CoreKitFrontend "Menu Widget Class" (Config/DefaultGame.ini). CoreKitFrontend owns the title-screen state (New Game,
 * Return to Main Menu, travel); this empty host only pushes UCoreKitBasicUIMainMenu on the CoreKitNavigation stack and removes the
 * front-end screens when the frontend removes it (travel).
 */
UCLASS()
class UCoreKitBasicUIFrontEndHost : public UCoreKitFrontendMenu
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIMainMenu* GetMainMenu() const { return MainMenu; }

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	UWidget* GetInitialFocus_Implementation() const override;

private:
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIMainMenu> MainMenu;
};
