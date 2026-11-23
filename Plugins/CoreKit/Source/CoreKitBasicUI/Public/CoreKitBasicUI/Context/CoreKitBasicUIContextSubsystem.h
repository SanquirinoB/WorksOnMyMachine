#pragma once

#include "Containers/Ticker.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CoreKitBasicUI/ViewModels/CoreKitBasicUIViewModels.h"
#include "CoreKitBasicUIContextSubsystem.generated.h"

class UCoreKitBasicUIPresenter;
class UCoreKitBasicUIEventChannel;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoreKitBasicUICommandRequested, FName, Command);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FCoreKitBasicUICardChoiceHandler, int32, OfferIndex);

/**
 * Creates the per-domain ViewModels for a local player, registers them in the global MVVM collection
 * (contexts CoreKitBasicUI.Run, CoreKitBasicUI.Vitals, CoreKitBasicUI.Loadout, CoreKitBasicUI.Progression, CoreKitBasicUI.Build, CoreKitBasicUI.Boss, CoreKitBasicUI.Minimap, CoreKitBasicUI.Summary),
 * and (re)creates the presenters when the run world / pawn changes. It does no gameplay work itself.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIContextSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
    bool ShouldCreateSubsystem(UObject* Outer) const override;
    UPROPERTY(BlueprintAssignable, Category="CoreKit|BasicUI") FCoreKitBasicUICommandRequested OnCommandRequested;
    UFUNCTION(BlueprintCallable, Category="CoreKit|BasicUI") void SetCardChoiceHandler(FCoreKitBasicUICardChoiceHandler Handler) { CardChoiceHandler = Handler; }
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIRunViewModel* GetRun() const { return Run; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIVitalsViewModel* GetVitals() const { return Vitals; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUILoadoutViewModel* GetLoadout() const { return Loadout; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIProgressionViewModel* GetProgression() const { return Progression; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIBuildViewModel* GetBuild() const { return Build; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIBossViewModel* GetBoss() const { return Boss; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIMinimapViewModel* GetMinimap() const { return Minimap; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIRunSummaryViewModel* GetSummary() const { return Summary; }
	UFUNCTION(BlueprintPure, Category = "CoreKitBasicUI|UI") UCoreKitBasicUIPreferencesViewModel* GetPreferences() const { return Preferences; }
	UFUNCTION(BlueprintPure, Category="CoreKit|BasicUI")
	UCoreKitBasicUIEventChannel* GetEvents() const;

	/** Called by the HUD when the run world is ready (and on Retry). Rebuilds presenters against the current pawn. */
	void StartRunPresentation(APlayerController* Controller);
	void StopRunPresentation();

	/** Presenters of the current run (empty outside a run). */
	const TArray<TObjectPtr<UCoreKitBasicUIPresenter>>& GetPresenters() const { return Presenters; }
	/** Ticks every presenter once now (tests, or after a forced gameplay change). */
	void TickPresenters(float DeltaSeconds);

	/** Automation: a context with its ViewModels but no local player (no event channel, no MVVM registration, no ticker). */
	static UCoreKitBasicUIContextSubsystem* CreateDetachedForTests(UObject* Outer);

private:
    UPROPERTY(Transient) FCoreKitBasicUICardChoiceHandler CardChoiceHandler;
	bool TickFromTicker(float DeltaSeconds);
	void CreateViewModels();
	bool bDetached = false;
	FTSTicker::FDelegateHandle PresenterTicker;
	void RegisterViewModels();
	void UnregisterViewModels();

	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIRunViewModel> Run;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIVitalsViewModel> Vitals;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUILoadoutViewModel> Loadout;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIProgressionViewModel> Progression;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIBuildViewModel> Build;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIBossViewModel> Boss;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIMinimapViewModel> Minimap;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIRunSummaryViewModel> Summary;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIPreferencesViewModel> Preferences;

	UPROPERTY(Transient) TArray<TObjectPtr<UCoreKitBasicUIPresenter>> Presenters;
	bool bRegistered = false;
};
