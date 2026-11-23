#include "CoreKitBasicUI/Presenters/CoreKitBasicUIPresenter.h"

#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitBasicUI/Events/CoreKitBasicUIEvents.h"
#include "GameFramework/PlayerController.h"

void UCoreKitBasicUIPresenter::Start(UCoreKitBasicUIContextSubsystem* InContext, APlayerController* InController)
{
	Context = InContext;
	Controller = InController;
	ReceiveStart(InContext, InController);
}

void UCoreKitBasicUIPresenter::Stop()
{
	ReceiveStop();
	Controller.Reset();
	Context.Reset();
}

UCoreKitBasicUIEventChannel* UCoreKitBasicUIPresenter::GetEvents() const
{
	return Context.IsValid() ? Context->GetEvents() : nullptr;
}

UWorld* UCoreKitBasicUIPresenter::GetPresenterWorld() const
{
	return Controller.IsValid() ? Controller->GetWorld() : nullptr;
}
