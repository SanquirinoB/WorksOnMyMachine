#pragma once

#include "UObject/Object.h"
#include "CoreKitBasicUIUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoreKitBasicUISettingsApplied, UCoreKitBasicUIUserSettings*, Settings, const UObject*, WorldContext);

/**
 * Player settings of Docs/UI/Spec/05-FrontEnd.md (owned by the model stage, UI-1). The Settings screen edits these
 * values live, Apply() pushes them to the engine, the player and the UI preferences, Save() persists them.
 */
UCLASS(Config = GameUserSettings, configdonotcheckdefaults)
class COREKITBASICUI_API UCoreKitBasicUIUserSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category="CoreKit|BasicUI") FCoreKitBasicUISettingsApplied OnApplied;
	UFUNCTION(BlueprintPure, Category="CoreKit|BasicUI")
	static UCoreKitBasicUIUserSettings* Get();

	// Gameplay
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float MouseSensitivity = 45.f;      // 0..100, shown as 0.45
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bInvertLookY = false;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float FieldOfView = 100.f;          // 80..120
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bDashBackwardWithoutInput = false;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bDamageNumbers = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float ScreenShake = 60.f;           // 0..100
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float HudScale = 100.f;             // 75..125
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 CrosshairColor = 0;           // 0 White, 1 Magenta, 2 Lime, 3 Mint
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bReducedMotion = false;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 GamepadGlyphs = 0;            // 0 Auto, 1 Xbox, 2 PlayStation
	// Video (UGameUserSettings holds the engine values; these mirror the UI choices)
	// Default Borderless at the desktop resolution: exclusive fullscreen at a fixed 1920x1080 switched the monitor mode on
	// first launch and flickered as focus moved (exclusive mode stays available in Settings).
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 DisplayMode = 1;              // 0 Fullscreen, 1 Borderless, 2 Windowed
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") FIntPoint Resolution = FIntPoint(0, 0);   // 0 x 0: the desktop resolution, resolved on first apply
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") int32 FrameRateLimit = 144;         // 0 = unlimited
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bVSync = false;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bMotionBlur = false;
	// Audio (0..100)
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float MasterVolume = 80.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float MusicVolume = 60.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") float EffectsVolume = 90.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bLowHealthHeartbeat = true;
	/** Set once the player applied video settings; until then the engine/command line video state is left alone. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="CoreKit|BasicUI") bool bVideoApplied = false;

	/** Applies every value to the engine, the player and the UI preferences. */
	void Apply(const UObject* WorldContext);
	/** Everything except video (resolution/window/frame limit/V-Sync): used at game start and at every run start. */
	void ApplyNonVideo(const UObject* WorldContext);
	/**
	 * Game start: ApplyNonVideo, plus the default video settings on the very first packaged launch (never in the editor
	 * or when the command line decides the window: -windowed, -RenderOffscreen, -unattended, -ResX...).
	 */
	void ApplyStartup(const UObject* WorldContext);
	/** Video values through UGameUserSettings (resolution and window mode are skipped inside the editor). */
	void ApplyVideo();
	/** Audio volumes for the world's audio device (CoreKitFrontend sound mix/classes when configured, else a CoreKitBasicUI mix). */
	void ApplyAudio(const UObject* WorldContext);
	/** Gameplay-facing scalars derived from the values. */
	float GetLookScale() const { return FMath::Max(MouseSensitivity, 1.f) / 45.f; }
	float GetScreenShakeScale() const { return FMath::Clamp(ScreenShake / 100.f, 0.f, 1.f); }
	float GetHudScaleFactor() const { return FMath::Clamp(HudScale, 75.f, 125.f) / 100.f; }
	/** The run camera's hip-fire FOV in degrees (80..120). */
	float GetFieldOfViewDegrees() const { return FMath::Clamp(FieldOfView, 80.f, 120.f); }
	void Save();
	void ResetToDefaults();
	FLinearColor GetCrosshairLinearColor() const;
};
