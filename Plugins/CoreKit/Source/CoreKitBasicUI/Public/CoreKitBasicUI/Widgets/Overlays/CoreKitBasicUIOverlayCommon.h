#pragma once

#include "CoreMinimal.h"
#include "CoreKitBasicUI/Style/CoreKitBasicUIStyle.h"

class UCanvasPanel;
class UImage;
class UUserWidget;
class UWidget;
class UWidgetTree;
class UCoreKitBasicUIScreenRouter;
class UCoreKitBasicUIActionButton;
class UTexture2D;

/**
 * Helpers shared by the overlays (Card Draw, Pause, confirmation prompt, Victory, Defeat) and the card component:
 * chamfer clipping without materials, outlines, text formatting of the spec and the Card Draw flow.
 */
namespace CoreKitBasicUIOverlay
{
	/** 1920x1080 reference size of the mockups. */
	inline const FVector2D Reference(1920.f, 1080.f);

	/** Brush that draws only a Px-wide border in Color (DrawAs Border over T_UI_White). */
	COREKITBASICUI_API FSlateBrush Outline(const FLinearColor& Color, float Px);

	/**
	 * Adds a canvas clipped to a rectangle whose top-left corner is cut at 45 degrees (Cut px), at Position/Size on
	 * Parent. Children of the returned canvas use the rectangle's local coordinates (0..Size). Implemented with two
	 * nested clips: an axis-aligned one and one rotated by -45 degrees (Slate stencil clipping), so no material is
	 * needed. Cut <= 0 gives a plain rectangle clip.
	 */
	COREKITBASICUI_API UCanvasPanel* AddChamferClip(UWidgetTree* Tree, UCanvasPanel* Parent, FVector2D Position, FVector2D Size, float Cut, FName Name = NAME_None, int32 ZOrder = 0);

	/** Thin outline following a chamfered rectangle (4 edges + the 45-degree cut). Returns the edge images. */
	COREKITBASICUI_API TArray<UImage*> AddChamferOutline(UWidgetTree* Tree, UCanvasPanel* Parent, FVector2D Position, FVector2D Size, float Cut, float Thickness, const FLinearColor& Color, int32 ZOrder = 0);

	/** "mm:ss" (minutes not wrapped). */
	COREKITBASICUI_API FString Clock(float Seconds);
	/** "07" (two digits, more when needed). */
	COREKITBASICUI_API FString TwoDigits(int32 Value);

	/** Confirmation bodies with real run data (Docs/UI/Spec/04-Overlays.md). */
	COREKITBASICUI_API FText RestartBody(float RunSeconds);
	COREKITBASICUI_API FText QuitToMenuBody(float RunSeconds, int32 Level, int32 Cards);

	/** One word of an effect sentence, split in runs; bKey runs use the accent style. */
	struct FEffectRun
	{
		FString Text;
		bool bKey = false;
	};
	using FEffectWord = TArray<FEffectRun>;

	/** Splits Pre + Key + Post into words (keeps punctuation attached), marking the runs of Key. */
	COREKITBASICUI_API TArray<FEffectWord> SplitEffect(const FString& Pre, const FString& Key, const FString& Post);

	/** UV region (0..1) of a crop of CropPx scene px centred on Focus (UV), shifted to stay inside the texture. */
	COREKITBASICUI_API FBox2f CropUV(FVector2D Focus, FVector2D CropPx, FVector2D ScenePx = FVector2D(352.f, 250.f));

	/** Loads a card art texture if its package exists (no warning when the art is not rendered yet). */
	COREKITBASICUI_API UTexture2D* LoadCardArt(const TSoftObjectPtr<UTexture2D>& Art);

	/** The run's screen router, through the owning player's ACoreKitBasicUIHUD (null outside a run). */
	COREKITBASICUI_API UCoreKitBasicUIScreenRouter* FindRouter(const UUserWidget* Widget);

	/** Default stage names when the Run ViewModel has none yet. */
	COREKITBASICUI_API FText StageName(int32 StageIndex);

	/** Low-pass snapshot on the music class while Pause or a modal is open (Docs/UI/Spec/08-Audio.md § Mezcla). */
	COREKITBASICUI_API void PushLowPass(const UObject* WorldContext);
	COREKITBASICUI_API void PopLowPass(const UObject* WorldContext);

	/**
	 * Offset that turns Slate's text line box (font ascent + descent) into the CSS line box of the mockups
	 * (line-height = CssLineHeightPx): (CssLineHeightPx - SlateLineHeight) / 2, negative for tight display titles.
	 * Use it as top and bottom padding (or as a Y offset) so big titles keep the mockup spacing.
	 */
	COREKITBASICUI_API float CssLineOffset(const FSlateFontInfo& Font, float CssLineHeightPx);

	/** Makes a design-system button's plate fill the button (UButtonSlot centres content by default). */
	COREKITBASICUI_API void FillButtonContent(UCoreKitBasicUIActionButton* Button);

	/** Sets translation, uniform scale, angle and render opacity in one call. */
	COREKITBASICUI_API void Pose(UWidget* Widget, FVector2D Offset, float Scale, float AngleDeg, float Opacity);
}

/**
 * Card Draw decision state (D6): the 400 ms lock starts when the cards finish dealing; one pick per offer and one
 * commit per pick. Pure logic so it can be tested without widgets.
 */
struct COREKITBASICUI_API FCoreKitBasicUICardDrawFlow
{
	enum class EState : uint8
	{
		/** XP fill, title, deal. Picks are rejected. */
		Intro,
		/** Cards are on the table; picks are accepted after LockSeconds. */
		Choose,
		/** A card was picked; the commit happens when it reaches the hand. */
		Picking,
		/** The pick was committed; waiting for a chained offer or closing. */
		Committed
	};

	EState State = EState::Intro;
	float LockSeconds = 0.4f;
	float ChooseElapsed = 0.f;
	float PickElapsed = 0.f;
	int32 Picked = INDEX_NONE;
	int32 Commits = 0;

	void Reset();
	void BeginChoose();
	void Tick(float DeltaSeconds);
	bool IsLocked() const { return State != EState::Choose || ChooseElapsed < LockSeconds; }
	/** Accepts a pick of an offered card once per offer. */
	bool TryPick(int32 Index, int32 NumOffers);
	/** True exactly once per pick, when the commit should be sent. */
	bool ConsumeCommit(float CommitDelaySeconds);
	/** The commit was rejected by gameplay: back to choosing (lock again). */
	void RevertPick();
};
