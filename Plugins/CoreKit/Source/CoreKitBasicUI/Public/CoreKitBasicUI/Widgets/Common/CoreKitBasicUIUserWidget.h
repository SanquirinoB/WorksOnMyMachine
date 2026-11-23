#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreKitNavigation/CoreKitMenuWidget.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIMotion.h"
#include "CoreKitBasicUIUserWidget.generated.h"

class UCoreKitBasicUIContextSubsystem;

/**
 * Base for CoreKitBasicUI widgets whose layout is built natively (BuildTree) when no Widget Blueprint tree exists.
 * Motion runs in NativeTick with real time (works while the game is paused). Subclasses bind ViewModels in
 * NativeConstruct through GetUIContext().
 */
UCLASS(Abstract)
class COREKITBASICUI_API UCoreKitBasicUIUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="CoreKit|BasicUI")
	UCoreKitBasicUIContextSubsystem* GetUIContext() const;

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	/** Build the widget tree here (WidgetTree is empty). */
	virtual void BuildTree() {}
	/** Real-time tick for tweens (unscaled, also while paused). */
	virtual void TickMotion(float RealDeltaSeconds) {}

	/** Plays a UI cue from /CoreKit/BasicUI/Audio (Docs/UI/Spec/08-Audio.md). */
	void PlayCue(FName Cue) const;

private:
	double LastRealTime = 0.0;
};

/** Base for CoreKitBasicUI menus managed by the CoreKitNavigation menu stack, with a native layout like UCoreKitBasicUIUserWidget. */
UCLASS(Abstract)
class COREKITBASICUI_API UCoreKitBasicUIMenuWidget : public UCoreKitMenuWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="CoreKit|BasicUI")
	UCoreKitBasicUIContextSubsystem* GetUIContext() const;

protected:
	TSharedRef<SWidget> RebuildWidget() override;
	void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	virtual void BuildTree() {}
	virtual void TickMotion(float RealDeltaSeconds) {}
	void PlayCue(FName Cue) const;

private:
	double LastRealTime = 0.0;
};

/** UI sound cues by name ("UI.Hover", "HUD.Kill"...); variants _1.._3 are picked at random. */
namespace CoreKitBasicUISound
{
	COREKITBASICUI_API void Play(const UObject* WorldContext, FName Cue, float Volume = 1.f);
	COREKITBASICUI_API class USoundBase* Find(FName Cue);
}
