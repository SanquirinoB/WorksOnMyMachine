#pragma once

#include "Components/Widget.h"
#include "CoreKitBasicUIArcWidget.generated.h"

class SCoreKitBasicUIArc;

/**
 * Ring / arc / conic progress drawn natively with Slate lines (no material): Dash ring, reload ring, charge and
 * lock-on rings, damage direction arc, slam rings. Angles in degrees, 0 = up, clockwise.
 */
UCLASS(meta = (DisplayName = "CoreKitBasicUI Arc"))
class COREKITBASICUI_API UCoreKitBasicUIArcWidget : public UWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetProgress(float InProgress);
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetArc(float InStartDeg, float InSweepDeg);
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetThickness(float InThickness);
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetArcColor(FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetTrackColor(FLinearColor InColor);
	/** Dashes along the arc (0 = solid). */
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetDashes(int32 InDashCount);
	/** Ellipse squash (1 = circle, 0.25 = floor ring seen at an angle). */
	UFUNCTION(BlueprintCallable, Category = "CoreKitBasicUI|UI") void SetSquash(float InSquash);

	float GetProgress() const { return Progress; }

	void SynchronizeProperties() override;
	void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(EditAnywhere, Category = "Arc") float Progress = 1.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float StartDeg = 0.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float SweepDeg = 360.f;
	UPROPERTY(EditAnywhere, Category = "Arc") float Thickness = 3.f;
	UPROPERTY(EditAnywhere, Category = "Arc") FLinearColor ArcColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category = "Arc") FLinearColor TrackColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Arc") int32 DashCount = 0;
	UPROPERTY(EditAnywhere, Category = "Arc") float Squash = 1.f;

private:
	TSharedPtr<SCoreKitBasicUIArc> Arc;
};
