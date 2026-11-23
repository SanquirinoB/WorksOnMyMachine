#pragma once

#include "Camera/CameraModifier.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CoreKitBasicUIScreenEffectsComponent.generated.h"

class UPostProcessComponent;
class UCoreKitBasicUIScreenEffectsCameraModifier;
struct FInstancedStruct;

/**
 * World-side screen effects driven by the UI (owned by the model stage, UI-1): post-process desaturation and glitch,
 * time dilation before the terminal pause, camera shakes and FOV kick scaled by Screen shake and Reduced motion.
 * Lives on the local player camera manager; get it with UCoreKitBasicUIScreenEffectsComponent::Get(WorldContext).
 *
 * Post-process runs on an unbound UPostProcessComponent attached to the camera manager, updated in real time also
 * while the world is paused (Card Draw freeze, terminal death look). FOV setting, damage shake and dash FOV kick run in
 * a camera modifier (UCoreKitBasicUIScreenEffectsCameraModifier). The component plays the damage shake on UI.Event.DamageTaken and
 * the dash kick on UI.Event.Dashed by itself; widgets may also call the functions directly.
 */
UCLASS(ClassGroup = (CoreKitBasicUI), meta = (BlueprintSpawnableComponent))
class COREKITBASICUI_API UCoreKitBasicUIScreenEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCoreKitBasicUIScreenEffectsComponent();

	static UCoreKitBasicUIScreenEffectsComponent* Get(const UObject* WorldContext);

	/** Card Draw freeze look: desaturate and darken the world (Alpha 0..1, animated by the caller). */
	void SetFreezeAmount(float Alpha);
	/** Death look: greyscale and darken (0..1). */
	void SetDeathAmount(float Alpha);
	/** Short RGB split / glitch pulse (seconds, capped at 0.15 s by the spec). Off with Reduced motion. */
	void PlayGlitch(float Seconds = 0.15f);
	/** Damage shake (6 px in the mockup) scaled by the Screen shake setting. Off with Reduced motion. */
	void PlayDamageShake();
	/** Dash FOV kick (4 %). Off with Reduced motion. */
	void PlayDashKick();
	/** Global time dilation (1 = normal). */
	void SetSlowMotion(float TimeDilation);

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Read by the camera modifier (real-time animated values).
	float GetShakeAlpha() const { return ShakeRemaining > 0.f ? ShakeRemaining / ShakeDuration : 0.f; }
	float GetKickAlpha() const { return KickRemaining > 0.f ? KickRemaining / KickDuration : 0.f; }
	float GetFreezeAmount() const { return FreezeAmount; }
	float GetDeathAmount() const { return DeathAmount; }
	bool IsGlitching() const { return GlitchRemaining > 0.f; }

	/** Tunables (spec: damage 420 ms SNAP, dash 260 ms SNAP, FOV +4 %). */
	UPROPERTY(EditAnywhere, Category = "Effects") float ShakeDuration = .42f;
	/** Peak view jitter in degrees at Screen shake 100 %. */
	UPROPERTY(EditAnywhere, Category = "Effects") float ShakeDegrees = .9f;
	UPROPERTY(EditAnywhere, Category = "Effects") float KickDuration = .26f;
	UPROPERTY(EditAnywhere, Category = "Effects") float KickFOVFraction = .04f;
	/** Freeze: brightness 0.5, saturation 0.6 (Card Draw); death: brightness 0.35, greyscale. */
	UPROPERTY(EditAnywhere, Category = "Effects") float FreezeBrightness = .5f;
	UPROPERTY(EditAnywhere, Category = "Effects") float FreezeSaturation = .6f;
	UPROPERTY(EditAnywhere, Category = "Effects") float DeathBrightness = .35f;
	UPROPERTY(EditAnywhere, Category = "Effects") float GlitchFringe = 5.f;

protected:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float FreezeAmount = 0.f;
	float DeathAmount = 0.f;
	float GlitchRemaining = 0.f;
	float GlitchDuration = .15f;
	float ShakeRemaining = 0.f;
	float KickRemaining = 0.f;

private:
	void UpdatePostProcess();
	void HandleEvent(FGameplayTag Event, const FInstancedStruct& Payload);

	UPROPERTY(Transient) TObjectPtr<UPostProcessComponent> PostProcess;
	UPROPERTY(Transient) TObjectPtr<UCoreKitBasicUIScreenEffectsCameraModifier> Modifier;
	FDelegateHandle DamageHandle, DashHandle;
};

/** Camera modifier of UCoreKitBasicUIScreenEffectsComponent: FOV setting (scales the character's FOV so ADS/sprint keep their ratio), shake and dash kick. */
UCLASS()
class COREKITBASICUI_API UCoreKitBasicUIScreenEffectsCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UCoreKitBasicUIScreenEffectsComponent> Effects;

protected:
	void ModifyCamera(float DeltaTime, FVector ViewLocation, FRotator ViewRotation, float FOV, FVector& NewViewLocation, FRotator& NewViewRotation, float& NewFOV) override;

private:
	float NoiseTime = 0.f;
};
