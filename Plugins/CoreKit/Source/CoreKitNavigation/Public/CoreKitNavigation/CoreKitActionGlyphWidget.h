#pragma once

#include "Components/Image.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitActionGlyphWidget.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

UCLASS(meta = (DisplayName = "Core Kit Action Glyph"))
class COREKITNAVIGATION_API UCoreKitActionGlyphWidget : public UImage
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetGlyphSource(UInputAction* InAction, UCoreKitNavigationGlyphs* InGlyphs);

	void SynchronizeProperties() override;
	void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	void BindPlayer();
	void UnbindPlayer();

	UFUNCTION()
	void HandleDeviceChanged(ECoreKitInputDevice Device);

	UFUNCTION()
	void HandleGamepadTypeChanged(FName GamepadType);

	UFUNCTION()
	void RefreshGlyph();

	UPROPERTY(EditAnywhere, Category = "CoreKit|Navigation")
	TObjectPtr<UInputAction> Action;

	UPROPERTY(EditAnywhere, Category = "CoreKit|Navigation")
	TObjectPtr<UCoreKitNavigationGlyphs> Glyphs;

	TWeakObjectPtr<UCoreKitNavigationSubsystem> Navigation;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> EnhancedInput;
};
