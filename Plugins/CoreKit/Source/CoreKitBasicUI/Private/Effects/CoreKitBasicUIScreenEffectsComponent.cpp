#include "CoreKitBasicUI/Effects/CoreKitBasicUIScreenEffectsComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PostProcessComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "CoreKitBasicUI/Events/CoreKitBasicUIEvents.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUIUserSettings.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"

UCoreKitBasicUIScreenEffectsComponent::UCoreKitBasicUIScreenEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

UCoreKitBasicUIScreenEffectsComponent* UCoreKitBasicUIScreenEffectsComponent::Get(const UObject* WorldContext)
{
	APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContext, 0);
	return Camera ? Camera->FindComponentByClass<UCoreKitBasicUIScreenEffectsComponent>() : nullptr;
}

void UCoreKitBasicUIScreenEffectsComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	PostProcess = NewObject<UPostProcessComponent>(Owner, TEXT("CoreKitBasicUIScreenEffectsPostProcess"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = 100.f;
	PostProcess->BlendWeight = 0.f;
	PostProcess->bEnabled = false;
	if (USceneComponent* Root = Owner->GetRootComponent())
	{
		PostProcess->SetupAttachment(Root);
	}
	PostProcess->RegisterComponent();

	APlayerCameraManager* Camera = Cast<APlayerCameraManager>(Owner);
	if (Camera)
	{
		Modifier = Cast<UCoreKitBasicUIScreenEffectsCameraModifier>(Camera->AddNewCameraModifier(UCoreKitBasicUIScreenEffectsCameraModifier::StaticClass()));
		if (Modifier)
		{
			Modifier->Effects = this;
		}
	}
	APlayerController* Controller = Camera ? Camera->GetOwningPlayerController() : nullptr;
	ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	if (UCoreKitBasicUIEventChannel* Events = Player ? Player->GetSubsystem<UCoreKitBasicUIEventChannel>() : nullptr)
	{
		DamageHandle = Events->Subscribe(CoreKitBasicUIEvent::DamageTaken, FCoreKitBasicUIEventDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleEvent));
		DashHandle = Events->Subscribe(CoreKitBasicUIEvent::Dashed, FCoreKitBasicUIEventDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleEvent));
	}
}

void UCoreKitBasicUIScreenEffectsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	APlayerCameraManager* Camera = Cast<APlayerCameraManager>(GetOwner());
	APlayerController* Controller = Camera ? Camera->GetOwningPlayerController() : nullptr;
	ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	if (UCoreKitBasicUIEventChannel* Events = Player ? Player->GetSubsystem<UCoreKitBasicUIEventChannel>() : nullptr)
	{
		Events->Unsubscribe(CoreKitBasicUIEvent::DamageTaken, DamageHandle);
		Events->Unsubscribe(CoreKitBasicUIEvent::Dashed, DashHandle);
	}
	if (Camera && Modifier)
	{
		Camera->RemoveCameraModifier(Modifier);
	}
	Modifier = nullptr;
	if (PostProcess)
	{
		PostProcess->DestroyComponent();
		PostProcess = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UCoreKitBasicUIScreenEffectsComponent::HandleEvent(FGameplayTag Event, const FInstancedStruct&)
{
	if (Event == CoreKitBasicUIEvent::DamageTaken)
	{
		PlayDamageShake();
		PlayGlitch(.15f);
	}
	else if (Event == CoreKitBasicUIEvent::Dashed)
	{
		PlayDashKick();
	}
}

void UCoreKitBasicUIScreenEffectsComponent::SetFreezeAmount(float Alpha)
{
	FreezeAmount = FMath::Clamp(Alpha, 0.f, 1.f);
	UpdatePostProcess();
}

void UCoreKitBasicUIScreenEffectsComponent::SetDeathAmount(float Alpha)
{
	DeathAmount = FMath::Clamp(Alpha, 0.f, 1.f);
	UpdatePostProcess();
}

void UCoreKitBasicUIScreenEffectsComponent::PlayGlitch(float Seconds)
{
	if (CoreKitBasicUIMotion::IsReducedMotion())
	{
		return;
	}
	GlitchDuration = FMath::Clamp(Seconds, .01f, .15f);
	GlitchRemaining = GlitchDuration;
	UpdatePostProcess();
}

void UCoreKitBasicUIScreenEffectsComponent::PlayDamageShake()
{
	if (CoreKitBasicUIMotion::IsReducedMotion() || UCoreKitBasicUIUserSettings::Get()->GetScreenShakeScale() <= 0.f)
	{
		return;
	}
	ShakeRemaining = ShakeDuration;
}

void UCoreKitBasicUIScreenEffectsComponent::PlayDashKick()
{
	if (CoreKitBasicUIMotion::IsReducedMotion())
	{
		return;
	}
	KickRemaining = KickDuration;
}

void UCoreKitBasicUIScreenEffectsComponent::SetSlowMotion(float TimeDilation)
{
	UGameplayStatics::SetGlobalTimeDilation(this, FMath::Clamp(TimeDilation, .0001f, 20.f));
}

void UCoreKitBasicUIScreenEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Real seconds: effects keep their spec timing under time dilation (death slow motion) and while paused.
	const float Real = FMath::Min(float(FApp::GetDeltaTime()), .1f);
	ShakeRemaining = FMath::Max(0.f, ShakeRemaining - Real);
	KickRemaining = FMath::Max(0.f, KickRemaining - Real);
	if (GlitchRemaining > 0.f)
	{
		GlitchRemaining = FMath::Max(0.f, GlitchRemaining - Real);
	}
	UpdatePostProcess();
}

void UCoreKitBasicUIScreenEffectsComponent::UpdatePostProcess()
{
	if (!PostProcess)
	{
		return;
	}
	const float Glitch = GlitchRemaining > 0.f ? GlitchRemaining / FMath::Max(.01f, GlitchDuration) : 0.f;
	const bool bActive = FreezeAmount > 0.f || DeathAmount > 0.f || Glitch > 0.f;
	PostProcess->bEnabled = bActive;
	PostProcess->BlendWeight = bActive ? 1.f : 0.f;
	if (!bActive)
	{
		return;
	}
	FPostProcessSettings& Settings = PostProcess->Settings;
	const float Saturation = FMath::Lerp(1.f, FreezeSaturation, FreezeAmount) * FMath::Lerp(1.f, 0.f, DeathAmount);
	const float Gain = FMath::Lerp(1.f, FreezeBrightness, FreezeAmount) * FMath::Lerp(1.f, DeathBrightness, DeathAmount);
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	Settings.bOverride_ColorGain = true;
	Settings.ColorGain = FVector4(Gain, Gain, Gain, 1.f);
	// Glitch: short chromatic aberration (scene fringe) that decays over the pulse.
	Settings.bOverride_SceneFringeIntensity = Glitch > 0.f;
	Settings.SceneFringeIntensity = GlitchFringe * Glitch;
}

void UCoreKitBasicUIScreenEffectsCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV)
{
	Super::ModifyCamera(DeltaTime, ViewLocation, ViewRotation, FOV, NewViewLocation, NewViewRotation, NewFOV);

	const UCoreKitBasicUIUserSettings* Settings = UCoreKitBasicUIUserSettings::Get();
	const UCoreKitBasicUIScreenEffectsComponent* Owner = Effects.Get();
	if (!Owner || CoreKitBasicUIMotion::IsReducedMotion())
	{
		return;
	}
	if (const float Kick = Owner->GetKickAlpha(); Kick > 0.f)
	{
		NewFOV *= 1.f + Owner->KickFOVFraction * Kick;
	}
	if (const float Shake = Owner->GetShakeAlpha(); Shake > 0.f)
	{
		NoiseTime += FMath::Min(float(FApp::GetDeltaTime()), .1f);
		const float Amplitude = Owner->ShakeDegrees * Settings->GetScreenShakeScale() * Shake * Shake;
		NewViewRotation.Pitch += Amplitude * FMath::Sin(NoiseTime * 71.f);
		NewViewRotation.Yaw += Amplitude * FMath::Sin(NoiseTime * 53.f + 1.3f);
	}
}
