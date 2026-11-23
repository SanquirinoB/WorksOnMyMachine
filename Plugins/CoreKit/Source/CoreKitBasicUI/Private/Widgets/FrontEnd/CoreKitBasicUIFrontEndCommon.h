#pragma once

#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIConfirmPrompt.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/FrontEnd/CoreKitBasicUIFrontEndMenus.h"

class UCanvasPanel;
class UCoreKitButton;
class UCoreKitMenuWidget;
class URichTextBlock;
class UWidgetTree;

/** Shared layout pieces of the front-end screens (private to CoreKitBasicUI). */
namespace CoreKitBasicUIFrontEndUI
{
	/** Canvas root with the Soot background and subtle scanlines of the full-screen menus. */
	UCanvasPanel* MakeScreenRoot(UWidgetTree* Tree, bool bOpaque = true);

	/** Kicker (JetBrains Mono 14, magenta) over a Saira Condensed 84 px title, as in FIELD MANUAL / SYSTEM CONFIG. */
	UWidget* MakeTitleBlock(UWidgetTree* Tree, const FText& Kicker, const FText& Title);

	/** Menu keycap (40 px high, 40 px min wide) with one key per device family. */
	UCoreKitBasicUIDeviceKeycap* MakeKeycap(UWidgetTree* Tree, FKey KeyboardKey, FKey GamepadKey, const FLinearColor& Accent = CoreKitBasicUIColor::Steel, float Height = 40.f);

	/** Keycap + label hint ("ESC BACK"); returns a non-focusable clear button wrapping it when OutButton is given. */
	UWidget* MakeKeyHint(UWidgetTree* Tree, FKey KeyboardKey, FKey GamepadKey, const FText& Label, const FLinearColor& LabelColor,
		const FLinearColor& Accent = CoreKitBasicUIColor::Steel, UCoreKitButton** OutButton = nullptr, UTextBlock** OutLabel = nullptr, UCoreKitBasicUIDeviceKeycap** OutKeycap = nullptr);

	/** Section heading (JetBrains Mono 13, 0.22 em, Muted). */
	UTextBlock* MakeHeading(UWidgetTree* Tree, const FText& Text);

	/** Wrapped paragraph. */
	UTextBlock* MakeParagraph(UWidgetTree* Tree, const FText& Text, const CoreKitBasicUIBuild::FTextStyle& Style, float WrapAt = 0.f);

	/**
	 * Rich text with inline colour runs: <m>magenta</>, <o>orange</>, <l>lime</>, <t>mint</>, <w>white</>, <b>bold white</>.
	 * Base style for the rest.
	 */
	URichTextBlock* MakeRich(UWidgetTree* Tree, const FText& Markup, const CoreKitBasicUIBuild::FTextStyle& Base);

	/** 1 px horizontal rule. */
	UWidget* MakeRule(UWidgetTree* Tree, const FLinearColor& Color, float Thickness = 1.f);

	/** Clear, non-focusable button (for clickable footer hints). */
	UCoreKitButton* MakeClearButton(UWidgetTree* Tree, UWidget* Content, bool bFocusable, FName Name = NAME_None);

	/** Styled confirmation (UCoreKitBasicUIConfirmPrompt of the overlays stage) with a preset's title/action and our body text. */
	void Confirm(UUserWidget* Owner, ECoreKitBasicUIPromptPreset Preset, const FText& Body, TFunction<void(bool)> OnDecided);

	/** Entry motion helper: eased 0..1 for a step that starts at Start seconds and lasts Duration (token curve). */
	float Step(float Time, float Start, float Duration, ECoreKitBasicUIMotion Curve);

	/** Applies opacity + translation to a widget (reduced motion keeps the fade, drops the offset). */
	void Enter(UWidget* Widget, float Alpha, FVector2D From);

	/** Plays the screen-change wipe for the player owning Widget. */
	void Wipe(const UUserWidget* Widget);
}
