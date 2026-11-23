#pragma once
#include "Containers/Ticker.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoreKitBasicUIMenuGameMode.generated.h"

class ACameraActor;
class UAudioComponent;
class UInputMappingContext;

/**
 * Game mode of L_MainMenu: no pawn, a menu controller that looks through the level's cinematic camera. The title
 * screen itself is presented by CoreKitFrontend (Menu Widget Class = UCoreKitBasicUIFrontEndHost, which pushes UCoreKitBasicUIMainMenu).
 */
UCLASS()
class COREKITBASICUI_API ACoreKitBasicUIMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACoreKitBasicUIMenuGameMode();

protected:
	void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};

/**
 * Menu controller: views the camera tagged "MenuCamera" (or the first camera actor) with a slow cinematic drift,
 * registers IMC_Combat so keycaps and Settings show the player's bindings, and plays the menu music (D4: the combat
 * track of DA_RunMusic through a low-pass filter at volume 0.4; UCoreKitBasicUIRunMusicSubsystem is not involved).
 */
UCLASS()
class COREKITBASICUI_API ACoreKitBasicUIMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACoreKitBasicUIMenuPlayerController();

	void FadeOutMusic(float Seconds);
	/** Master x Music of the player's settings (0..1). */
	static float UserMusicVolume();
	UAudioComponent* GetMusic() const { return Music; }
	ACameraActor* GetMenuCamera() const { return MenuCamera.Get(); }

	/** Menu music settings (D4). */
	UPROPERTY(EditAnywhere, Category = "Menu|Music", meta = (ClampMin = "0", ClampMax = "1")) float MusicVolume = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Menu|Music", meta = (ClampMin = "100", Units = "Hz")) float LowPassFrequency = 900.f;
	UPROPERTY(EditAnywhere, Category = "Menu|Music", meta = (ClampMin = "0", Units = "s")) float MusicFadeIn = 1.5f;
	/** Camera drift: lateral sway (cm), yaw sway (degrees) and period (s). */
	UPROPERTY(EditAnywhere, Category = "Menu|Camera") float DriftDistance = 90.f;
	UPROPERTY(EditAnywhere, Category = "Menu|Camera") float DriftYaw = 2.5f;
	UPROPERTY(EditAnywhere, Category = "Menu|Camera", meta = (ClampMin = "1")) float DriftPeriod = 24.f;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type Reason) override;
	void Tick(float DeltaSeconds) override;

private:
	void StartMusic();
	void RegisterBindings();

	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
	UPROPERTY(Transient) TObjectPtr<class USoundClass> MusicClass;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Mapping;
	TWeakObjectPtr<ACameraActor> MenuCamera;
	FTransform CameraBase;
	double DriftStart = 0.0;
};

/**
 * Front-end flow glue: when a CoreKitLoading screen goes away (New Game, Return to Main Menu) the arriving screen is
 * revealed with the slash wipe. Created only while CoreKitFrontend is enabled.
 */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIFrontEndFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool ShouldCreateSubsystem(UObject* Outer) const override;
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

	int32 GetRevealCount() const { return RevealCount; }
	float GetLastLoadingHoldSeconds() const { return LastLoadingHoldSeconds; }

private:
	UFUNCTION() void HandleLoadingScreenVisibility(bool bVisible);
	void HandleLoadingMapLoaded(UWorld* World);

	int32 RevealCount = 0;
	FGuid LoadingHoldRequest;
	FDelegateHandle LoadingMapHandle;
	FTSTicker::FDelegateHandle LoadingHoldTicker;
	double LoadingMapReadyAt = 0.0;
	float LastLoadingHoldSeconds = 0.f;
	bool bWaitingForLoadingMap = false;
};
