#include "CoreKitBasicUI/Settings/CoreKitBasicUIUserSettings.h"

#include "AudioDevice.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"
#include "CoreKitFrontend/CoreKitFrontendSettings.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Sound/AudioSettings.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	UGameInstance* GameInstanceOf(const UObject* WorldContext)
	{
		if (const ULocalPlayer* Player = Cast<ULocalPlayer>(WorldContext))
		{
			return Player->GetGameInstance();
		}
		const UWorld* World = GEngine && WorldContext ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
		return World ? World->GetGameInstance() : nullptr;
	}

	/** Local players of the context's game instance, or of every game world when no context is given. */
	TArray<ULocalPlayer*> LocalPlayersOf(const UObject* WorldContext)
	{
		TArray<ULocalPlayer*> Players;
		if (UGameInstance* Instance = GameInstanceOf(WorldContext))
		{
			Players.Append(Instance->GetLocalPlayers());
			return Players;
		}
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.OwningGameInstance && (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE))
				{
					Players.Append(Context.OwningGameInstance->GetLocalPlayers());
				}
			}
		}
		return Players;
	}

	UWorld* WorldOf(const UObject* WorldContext)
	{
		if (const ULocalPlayer* Player = Cast<ULocalPlayer>(WorldContext))
		{
			return Player->GetWorld();
		}
		return GEngine && WorldContext ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	}

	/** Command lines that already decide the window (validation, offscreen captures, explicit resolution). */
	bool CommandLineOwnsVideo()
	{
		const TCHAR* Line = FCommandLine::Get();
		int32 Value = 0;
		return FParse::Param(Line, TEXT("windowed")) || FParse::Param(Line, TEXT("fullscreen")) || FParse::Param(Line, TEXT("RenderOffscreen"))
			|| FParse::Param(Line, TEXT("unattended")) || FParse::Param(Line, TEXT("nullrhi")) || FParse::Value(Line, TEXT("ResX="), Value);
	}

	USoundMix* CoreKitBasicUIVolumeMix()
	{
		static TStrongObjectPtr<USoundMix> Mix;
		if (!Mix)
		{
			Mix.Reset(NewObject<USoundMix>(GetTransientPackage(), TEXT("CoreKitBasicUI_UserVolumeMix")));
		}
		return Mix.Get();
	}
}

UCoreKitBasicUIUserSettings* UCoreKitBasicUIUserSettings::Get()
{
	static TStrongObjectPtr<UCoreKitBasicUIUserSettings> Instance;
	if (!Instance)
	{
		Instance.Reset(NewObject<UCoreKitBasicUIUserSettings>());
		Instance->LoadConfig();
		// Mirror what the engine really runs once the player has chosen video settings before.
		if (Instance->bVideoApplied && GEngine && GEngine->GetGameUserSettings())
		{
			const UGameUserSettings* Video = GEngine->GetGameUserSettings();
			const EWindowMode::Type Mode = Video->GetFullscreenMode();
			Instance->DisplayMode = Mode == EWindowMode::Fullscreen ? 0 : Mode == EWindowMode::WindowedFullscreen ? 1 : 2;
			Instance->Resolution = Video->GetScreenResolution();
			Instance->FrameRateLimit = FMath::RoundToInt(Video->GetFrameRateLimit());
			Instance->bVSync = Video->IsVSyncEnabled();
		}
	}
	return Instance.Get();
}

void UCoreKitBasicUIUserSettings::Apply(const UObject* WorldContext)
{
	ApplyNonVideo(WorldContext);
	ApplyVideo();
}

void UCoreKitBasicUIUserSettings::ApplyStartup(const UObject* WorldContext)
{
	ApplyNonVideo(WorldContext);
	if (!bVideoApplied && !GIsEditor && !CommandLineOwnsVideo())
	{
		ApplyVideo();
		Save();
	}
}

void UCoreKitBasicUIUserSettings::ApplyNonVideo(const UObject* WorldContext)
{
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.f, 100.f);
	FieldOfView = FMath::Clamp(FieldOfView, 80.f, 120.f);
	ScreenShake = FMath::Clamp(ScreenShake, 0.f, 100.f);
	HudScale = FMath::Clamp(HudScale, 75.f, 125.f);
	CrosshairColor = FMath::Clamp(CrosshairColor, 0, 3);
	GamepadGlyphs = FMath::Clamp(GamepadGlyphs, 0, 2);
	MasterVolume = FMath::Clamp(MasterVolume, 0.f, 100.f);
	MusicVolume = FMath::Clamp(MusicVolume, 0.f, 100.f);
	EffectsVolume = FMath::Clamp(EffectsVolume, 0.f, 100.f);

	// Gameplay hooks (CoreKitBasicUI): look modifier on IA_Look, dash direction, damage numbers on the FloatingText consumers.
	// FOV and screen shake are read every frame by UCoreKitBasicUIScreenEffectsComponent's camera modifier.
	
	
	
	CoreKitBasicUIMotion::SetReducedMotion(bReducedMotion);

	if (IConsoleVariable* MotionBlur = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
	{
		MotionBlur->Set(bMotionBlur ? 4 : 0, ECVF_SetByGameSetting);
	}

	const FName GamepadType = GamepadGlyphs == 1 ? FName(TEXT("Xbox")) : GamepadGlyphs == 2 ? FName(TEXT("PlayStation")) : NAME_None;
	for (ULocalPlayer* Player : LocalPlayersOf(WorldContext))
	{
		if (!Player) continue;
		if (UCoreKitBasicUIContextSubsystem* Context = Player->GetSubsystem<UCoreKitBasicUIContextSubsystem>())
		{
			UCoreKitBasicUIPreferencesViewModel* Preferences = Context->GetPreferences();
			Preferences->SetHudScale(GetHudScaleFactor());
			Preferences->SetCrosshairColor(GetCrosshairLinearColor());
			Preferences->SetbReducedMotion(bReducedMotion);
			Preferences->SetbLowHealthHeartbeat(bLowHealthHeartbeat);
			Preferences->SetbDamageNumbers(bDamageNumbers);
			Preferences->SetScreenShake(GetScreenShakeScale());
			Context->GetLoadout()->SetCrosshairColor(GetCrosshairLinearColor());
		}
		if (UCoreKitNavigationSubsystem* Navigation = Player->GetSubsystem<UCoreKitNavigationSubsystem>())
		{
			if (Navigation->GetGamepadTypeOverride() != GamepadType)
			{
				Navigation->SetGamepadTypeOverride(GamepadType);
			}
		}
	}
	ApplyAudio(WorldContext);
	OnApplied.Broadcast(this, WorldContext);
}

void UCoreKitBasicUIUserSettings::ApplyVideo()
{
	UGameUserSettings* Video = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Video)
	{
		return;
	}
	if (Resolution.X <= 0 || Resolution.Y <= 0 || DisplayMode == 1)
	{
		// Borderless always covers the desktop; an unset resolution also starts from the desktop.
		Resolution = Video->GetDesktopResolution();
	}
	Video->SetFullscreenMode(DisplayMode == 0 ? EWindowMode::Fullscreen : DisplayMode == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	if (Resolution.X > 0 && Resolution.Y > 0)
	{
		Video->SetScreenResolution(Resolution);
	}
	Video->SetFrameRateLimit(FrameRateLimit > 0 ? float(FrameRateLimit) : 0.f);
	Video->SetVSyncEnabled(bVSync);
	// The editor owns its window: only frame limit and V-Sync apply there.
	if (!GIsEditor)
	{
		Video->ApplyResolutionSettings(true);
	}
	Video->ApplyNonResolutionSettings();
	Video->SaveSettings();
	bVideoApplied = true;
}

void UCoreKitBasicUIUserSettings::ApplyAudio(const UObject* WorldContext)
{
	UWorld* World = WorldOf(WorldContext);
	if (!World || !World->GetAudioDevice().IsValid())
	{
		return;
	}
	const float Master = MasterVolume / 100.f;
	const float Music = MusicVolume / 100.f;
	const float Effects = EffectsVolume / 100.f;

	USoundMix* Mix = nullptr;
	const UCoreKitFrontendSettings* Frontend = GetDefault<UCoreKitFrontendSettings>();
	USoundMix* FrontendMix = Frontend->UserVolumeMix.LoadSynchronous();
	USoundClass* FrontendMaster = Frontend->MasterSoundClass.LoadSynchronous();
	if (FrontendMix && FrontendMaster)
	{
		// Project-configured mix and classes (CoreKitFrontend): master, music and effects adjusters.
		Mix = FrontendMix;
		UGameplayStatics::SetSoundMixClassOverride(World, Mix, FrontendMaster, Master, 1.f, 0.f, true);
		if (USoundClass* MusicClass = Frontend->MusicSoundClass.LoadSynchronous()) UGameplayStatics::SetSoundMixClassOverride(World, Mix, MusicClass, Music, 1.f, 0.f, true);
		if (USoundClass* EffectsClass = Frontend->EffectsSoundClass.LoadSynchronous()) UGameplayStatics::SetSoundMixClassOverride(World, Mix, EffectsClass, Effects, 1.f, 0.f, true);
	}
	else
	{
		// No configured classes: apply the default sound-class volume mix.
		Mix = CoreKitBasicUIVolumeMix();
		if (USoundClass* Default = GetDefault<UAudioSettings>()->GetDefaultSoundClass())
		{
			UGameplayStatics::SetSoundMixClassOverride(World, Mix, Default, Master * Effects, 1.f, 0.f, true);
		}
		// Run music retains the project-authored subsystem; no UI music-voice overrides.

	}
	// Push once per audio device (mix modifiers are reference counted and persist across levels).
	static TSet<uint32> PushedDevices;
	const uint32 Device = World->GetAudioDevice().GetDeviceID();
	if (!PushedDevices.Contains(Device))
	{
		PushedDevices.Add(Device);
		UGameplayStatics::PushSoundMixModifier(World, Mix);
	}
}

void UCoreKitBasicUIUserSettings::Save()
{
	SaveConfig();
}

void UCoreKitBasicUIUserSettings::ResetToDefaults()
{
	const UCoreKitBasicUIUserSettings* Defaults = GetDefault<UCoreKitBasicUIUserSettings>();
	const bool bKeepVideoApplied = bVideoApplied;
	for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_Config)) It->CopyCompleteValue_InContainer(this, Defaults);
	}
	bVideoApplied = bKeepVideoApplied;
}

FLinearColor UCoreKitBasicUIUserSettings::GetCrosshairLinearColor() const
{
	switch (CrosshairColor)
	{
	case 1: return CoreKitBasicUIColor::Magenta;
	case 2: return CoreKitBasicUIColor::Lime;
	case 3: return CoreKitBasicUIColor::Mint;
	default: return CoreKitBasicUIColor::White;
	}
}
