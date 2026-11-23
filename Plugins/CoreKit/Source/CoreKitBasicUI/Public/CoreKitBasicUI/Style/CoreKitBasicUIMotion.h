#pragma once

#include "Engine/DeveloperSettings.h"
#include "CoreKitBasicUIMotion.generated.h"

/** Motion tokens of Docs/UI/Spec/07-Motion.md. */
UENUM(BlueprintType)
enum class ECoreKitBasicUIMotion : uint8
{
	Snap,
	Settle,
	Slam,
	Deal,
	Exit,
	Pulse,
	Linear
};

/** Editable motion token durations (Project Settings > Game > CoreKitBasicUI Motion). Curves are the spec's cubic-beziers. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CoreKitBasicUI Motion"))
class COREKITBASICUI_API UCoreKitBasicUIMotionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float Snap = 0.09f;
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float Settle = 0.2f;
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float Slam = 0.38f;
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float Deal = 0.52f;
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float DealStagger = 0.07f;
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float Exit = 0.14f;
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float Pulse = 1.f;
	/** Screen wipe between screens. */
	UPROPERTY(Config, EditAnywhere, Category = "Tokens", meta = (Units = "s")) float ScreenWipe = 0.32f;

	float Duration(ECoreKitBasicUIMotion Token) const;
};

/** Curve evaluation and a tiny value tween for NativeTick interpolation (UE standard: values driven per tick). */
namespace CoreKitBasicUIMotion
{
	/** Eased 0..1 for a token's curve. Slam overshoots above 1. Pulse is a sine 0..1..0 over its period. */
	COREKITBASICUI_API float Ease(ECoreKitBasicUIMotion Token, float Alpha);

	/** True when the player enabled Reduced motion (set by UCoreKitBasicUIUserSettings). */
	COREKITBASICUI_API bool IsReducedMotion();
	COREKITBASICUI_API void SetReducedMotion(bool bReduced);
}

/** One animated float. Call Tick with real (unpaused) delta time; read Value. */
USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUITween
{
	GENERATED_BODY()

	float From = 0.f;
	float To = 0.f;
	float Duration = 0.f;
	float Delay = 0.f;
	float Elapsed = 0.f;
	ECoreKitBasicUIMotion Curve = ECoreKitBasicUIMotion::Settle;
	bool bActive = false;
	float Value = 0.f;

	/** Starts from the current value (or From when bFromCurrent is false). Duration < 0 uses the token's duration. */
	void Start(float InTo, ECoreKitBasicUIMotion InCurve, float InDuration = -1.f, float InDelay = 0.f, bool bFromCurrent = true);
	void Start(float InFrom, float InTo, ECoreKitBasicUIMotion InCurve, float InDuration = -1.f, float InDelay = 0.f);
	void Snap(float InValue);
	/** Returns true while running. Reduced motion jumps to the end. */
	bool Tick(float DeltaSeconds);
	float Alpha() const { return Duration > 0.f ? FMath::Clamp((Elapsed - Delay) / Duration, 0.f, 1.f) : 1.f; }
	bool IsActive() const { return bActive; }
};
