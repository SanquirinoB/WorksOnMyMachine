#pragma once

#include "UObject/Object.h"
#include "CoreKitBasicUIPresenter.generated.h"

class APlayerController;
class UCoreKitBasicUIContextSubsystem;
class UCoreKitBasicUIEventChannel;

/**
 * One presenter per domain: observes gameplay (delegates or a light tick) and writes only its ViewModel.
 * Created by UCoreKitBasicUIContextSubsystem::StartRunPresentation; Stop() must unbind everything.
 */
UCLASS(Abstract, Blueprintable)
class COREKITBASICUI_API UCoreKitBasicUIPresenter : public UObject
{
	GENERATED_BODY()

public:
	virtual void Start(UCoreKitBasicUIContextSubsystem* InContext, APlayerController* InController);
	virtual void Stop();
	/** Real-time tick from the context (about 30 Hz), also while paused. */
	virtual void Tick(float DeltaSeconds) { ReceiveTick(DeltaSeconds); }
	UFUNCTION(BlueprintImplementableEvent, Category="CoreKit|BasicUI") void ReceiveStart(UCoreKitBasicUIContextSubsystem* UIContext, APlayerController* PlayerController);
	UFUNCTION(BlueprintImplementableEvent, Category="CoreKit|BasicUI") void ReceiveStop();
	UFUNCTION(BlueprintImplementableEvent, Category="CoreKit|BasicUI") void ReceiveTick(float DeltaSeconds);

protected:
	UCoreKitBasicUIContextSubsystem* GetContext() const { return Context.Get(); }
	APlayerController* GetController() const { return Controller.Get(); }
	UCoreKitBasicUIEventChannel* GetEvents() const;
	UWorld* GetPresenterWorld() const;

private:
	TWeakObjectPtr<UCoreKitBasicUIContextSubsystem> Context;
	TWeakObjectPtr<APlayerController> Controller;
};
