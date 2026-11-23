// Overlay stage (UI-3 / UI-4) tests: Card Draw flow, router decisions, prompt texts and card helpers.
#include "CoreKitBasicUI/Flow/CoreKitBasicUIScreenRouter.h"
#include "Misc/AutomationTest.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUICardWidget.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIConfirmPrompt.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIEndScreens.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitBasicUIOverlaysClockTest, "CoreKit.BasicUI.Overlays.ClockFormat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoreKitBasicUIOverlaysClockTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Zero"), CoreKitBasicUIOverlay::Clock(0.f), FString(TEXT("00:00")));
	TestEqual(TEXT("Floors seconds"), CoreKitBasicUIOverlay::Clock(342.9f), FString(TEXT("05:42")));
	TestEqual(TEXT("Ten minutes"), CoreKitBasicUIOverlay::Clock(600.f), FString(TEXT("10:00")));
	TestEqual(TEXT("Overtime"), CoreKitBasicUIOverlay::Clock(674.f), FString(TEXT("11:14")));
	TestEqual(TEXT("Negative clamps"), CoreKitBasicUIOverlay::Clock(-3.f), FString(TEXT("00:00")));
	TestEqual(TEXT("Two digits"), CoreKitBasicUIOverlay::TwoDigits(7), FString(TEXT("07")));
	TestEqual(TEXT("Three digits"), CoreKitBasicUIOverlay::TwoDigits(142), FString(TEXT("142")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitBasicUIOverlaysPromptTextsTest, "CoreKit.BasicUI.Overlays.PromptTexts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoreKitBasicUIOverlaysPromptTextsTest::RunTest(const FString& Parameters)
{
	// Literal texts of Docs/UI/Spec/04-Overlays.md with the mockup's data (05:42, level 07, 6 cards).
	TestEqual(TEXT("Restart body"), CoreKitBasicUIOverlay::RestartBody(342.f).ToString(),
		FString(TEXT("Your build and 05:42 of progress will be lost. A fresh run starts at 00:00.")));
	TestEqual(TEXT("Quit body"), CoreKitBasicUIOverlay::QuitToMenuBody(342.f, 7, 6).ToString(),
		FString(TEXT("You will lose this run: 05:42 survived, level 07 and 6 cards.")));
	TestEqual(TEXT("Quit body singular"), CoreKitBasicUIOverlay::QuitToMenuBody(61.f, 2, 1).ToString(),
		FString(TEXT("You will lose this run: 01:01 survived, level 02 and 1 card.")));

	struct FCase { ECoreKitBasicUIPromptPreset Preset; const TCHAR* Title; const TCHAR* Confirm; bool bBody; };
	const FCase Cases[] = {
		{ ECoreKitBasicUIPromptPreset::RestartRun, TEXT("Restart run?"), TEXT("RESTART"), true },
		{ ECoreKitBasicUIPromptPreset::QuitToMenu, TEXT("Quit to menu?"), TEXT("QUIT"), true },
		{ ECoreKitBasicUIPromptPreset::QuitToDesktop, TEXT("Quit to desktop?"), TEXT("QUIT"), false },
		{ ECoreKitBasicUIPromptPreset::ResetDefaults, TEXT("Reset to defaults?"), TEXT("RESET"), false },
		{ ECoreKitBasicUIPromptPreset::DiscardChanges, TEXT("Discard changes?"), TEXT("DISCARD"), false } };
	for (const FCase& Case : Cases)
	{
		FText Title, Body, Confirm;
		CoreKitBasicUIPrompt::PresetTexts(Case.Preset, 342.f, 7, 6, Title, Body, Confirm);
		TestEqual(FString::Printf(TEXT("%s title"), Case.Title), Title.ToString(), FString(Case.Title));
		TestEqual(FString::Printf(TEXT("%s action"), Case.Title), Confirm.ToString(), FString(Case.Confirm));
		TestEqual(FString::Printf(TEXT("%s body presence"), Case.Title), !Body.IsEmpty(), Case.bBody);
	}
	TestEqual(TEXT("Cancel is option 0"), UCoreKitBasicUIConfirmPrompt::CancelIndex, 0);
	TestEqual(TEXT("Action is option 1"), UCoreKitBasicUIConfirmPrompt::ConfirmIndex, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitBasicUIOverlaysEffectSplitTest, "CoreKit.BasicUI.Overlays.EffectSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoreKitBasicUIOverlaysEffectSplitTest::RunTest(const FString& Parameters)
{
	// "Bullets pass through every enemy in line." -> key = "pass through".
	const TArray<CoreKitBasicUIOverlay::FEffectWord> Piercing = CoreKitBasicUIOverlay::SplitEffect(TEXT("Bullets "), TEXT("pass through"), TEXT(" every enemy in line."));
	TestEqual(TEXT("Word count"), Piercing.Num(), 7);
	if (Piercing.Num() == 7)
	{
		TestFalse(TEXT("Bullets plain"), Piercing[0][0].bKey);
		TestTrue(TEXT("pass key"), Piercing[1][0].bKey);
		TestTrue(TEXT("through key"), Piercing[2][0].bKey);
		TestFalse(TEXT("every plain"), Piercing[3][0].bKey);
		TestEqual(TEXT("Punctuation stays on the last word"), Piercing[6][0].Text, FString(TEXT("line.")));
	}
	// "Every katana swing sends out a shockwave." -> last word mixes key + punctuation.
	const TArray<CoreKitBasicUIOverlay::FEffectWord> Shockwave = CoreKitBasicUIOverlay::SplitEffect(TEXT("Every katana swing sends out a "), TEXT("shockwave"), TEXT("."));
	TestTrue(TEXT("Has words"), Shockwave.Num() > 0);
	if (Shockwave.Num() > 0)
	{
		const CoreKitBasicUIOverlay::FEffectWord& Last = Shockwave.Last();
		TestEqual(TEXT("Mixed word has two runs"), Last.Num(), 2);
		if (Last.Num() == 2)
		{
			TestTrue(TEXT("Key run"), Last[0].bKey && Last[0].Text == TEXT("shockwave"));
			TestTrue(TEXT("Post run"), !Last[1].bKey && Last[1].Text == TEXT("."));
		}
	}
	// "+15% rifle damage." -> key first.
	const TArray<CoreKitBasicUIOverlay::FEffectWord> Firepower = CoreKitBasicUIOverlay::SplitEffect(TEXT(""), TEXT("+15%"), TEXT(" rifle damage."));
	TestTrue(TEXT("Key first"), Firepower.Num() == 3 && Firepower[0][0].bKey && Firepower[0][0].Text == TEXT("+15%"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitBasicUIOverlaysCropTest, "CoreKit.BasicUI.Overlays.CardCropAndSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoreKitBasicUIOverlaysCropTest::RunTest(const FString& Parameters)
{
	const FVector2D Scene(352.f, 250.f);
	const FBox2f Centered = CoreKitBasicUIOverlay::CropUV(FVector2D(0.5f, 0.5f), FVector2D(176.f, 125.f), Scene);
	TestTrue(TEXT("Centred crop"), Centered.Min.Equals(FVector2f(0.25f, 0.25f), 1e-3f) && Centered.Max.Equals(FVector2f(0.75f, 0.75f), 1e-3f));
	const FBox2f Edge = CoreKitBasicUIOverlay::CropUV(FVector2D(0.98f, 0.02f), FVector2D(176.f, 125.f), Scene);
	TestTrue(TEXT("Crop shifted inside the texture"), Edge.Min.X >= 0.f && Edge.Min.Y >= 0.f && Edge.Max.X <= 1.f + 1e-4f && Edge.Max.Y <= 1.f + 1e-4f);
	TestTrue(TEXT("Crop keeps its size at the edge"), FMath::IsNearlyEqual(Edge.Max.X - Edge.Min.X, 0.5f, 1e-3f));
	const FBox2f Oversized = CoreKitBasicUIOverlay::CropUV(FVector2D(0.5f, 0.5f), FVector2D(700.f, 500.f), Scene);
	TestTrue(TEXT("Oversized crop clamps to the whole texture"), Oversized.Min.Equals(FVector2f(0.f, 0.f), 1e-3f) && Oversized.Max.Equals(FVector2f(1.f, 1.f), 1e-3f));

	TestTrue(TEXT("Full"), UCoreKitBasicUICardWidget::VariantSize(ECoreKitBasicUICardVariant::Full).Equals(FVector2D(360.f, 540.f)));
	TestTrue(TEXT("Mini"), UCoreKitBasicUICardWidget::VariantSize(ECoreKitBasicUICardVariant::Mini).Equals(FVector2D(52.f, 72.f)));
	TestTrue(TEXT("Victory"), UCoreKitBasicUICardWidget::VariantSize(ECoreKitBasicUICardVariant::Victory).Equals(FVector2D(76.f, 100.f)));
	TestTrue(TEXT("Defeat"), UCoreKitBasicUICardWidget::VariantSize(ECoreKitBasicUICardVariant::Defeat).Equals(FVector2D(62.f, 82.f)));
	TestTrue(TEXT("Thumb 34"), UCoreKitBasicUICardWidget::VariantSize(ECoreKitBasicUICardVariant::Thumb, 34.f).Equals(FVector2D(34.f, 34.f)));

	FCoreKitBasicUICardData Special;
	Special.Suit = ECoreKitBasicUICardSuit::Sight;
	Special.bSpecial = true;
	TestTrue(TEXT("Specials use the magenta accent even on SIGHT"), Special.Accent().Equals(CoreKitBasicUIColor::Magenta));
	TestTrue(TEXT("Missing art loads as null (placeholder path)"), CoreKitBasicUIOverlay::LoadCardArt(TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/DeadDraw/UI/CardArt/T_CardArt_DoesNotExist_Scene.T_CardArt_DoesNotExist_Scene")))) == nullptr);
	TestTrue(TEXT("Ranged drone tip literal"), UCoreKitBasicUIDefeatScreen::DefaultTip(FText::FromString(TEXT("RANGED DRONE"))).ToString().StartsWith(TEXT("An orange ring around a drone")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitBasicUIOverlaysCardDrawFlowTest, "CoreKit.BasicUI.Overlays.CardDrawSingleCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoreKitBasicUIOverlaysCardDrawFlowTest::RunTest(const FString& Parameters)
{
	FCoreKitBasicUICardDrawFlow Flow;
	TestFalse(TEXT("Intro rejects picks"), Flow.TryPick(0, 3));
	Flow.BeginChoose();
	TestTrue(TEXT("Locked right after the deal"), Flow.IsLocked());
	Flow.Tick(0.39f);
	TestFalse(TEXT("Still locked at 390 ms"), Flow.TryPick(1, 3));
	Flow.Tick(0.02f);
	TestFalse(TEXT("Out of range"), Flow.TryPick(3, 3));
	TestTrue(TEXT("Accepted after 400 ms"), Flow.TryPick(1, 3));
	TestFalse(TEXT("Second pick rejected"), Flow.TryPick(2, 3));
	TestEqual(TEXT("Picked index"), Flow.Picked, 1);
	TestFalse(TEXT("No commit before the card reaches the hand"), Flow.ConsumeCommit(0.75f));
	Flow.Tick(0.76f);
	TestTrue(TEXT("Commit on arrival"), Flow.ConsumeCommit(0.75f));
	TestFalse(TEXT("Commit only once"), Flow.ConsumeCommit(0.75f));
	Flow.Tick(1.f);
	TestFalse(TEXT("Still only once"), Flow.ConsumeCommit(0.75f));
	TestEqual(TEXT("One commit per offer"), Flow.Commits, 1);

	// Rejected commit: back to choosing with a fresh lock.
	Flow.RevertPick();
	TestTrue(TEXT("Locked again after a revert"), Flow.IsLocked());
	Flow.Tick(0.5f);
	TestTrue(TEXT("Can pick again"), Flow.TryPick(0, 3));

	// Chained offer: a new offer starts a new intro.
	Flow.Reset();
	TestEqual(TEXT("Intro after reset"), static_cast<int32>(Flow.State), static_cast<int32>(FCoreKitBasicUICardDrawFlow::EState::Intro));
	TestFalse(TEXT("Reset rejects picks"), Flow.TryPick(0, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreKitBasicUIOverlaysRouterTest, "CoreKit.BasicUI.Overlays.RouterDecisions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoreKitBasicUIOverlaysRouterTest::RunTest(const FString& Parameters)
{
	auto Expect = [this](const TCHAR* What, const FCoreKitBasicUIRouterInputs& In, ECoreKitBasicUIRouterAction Expected)
	{
		TestEqual(What, static_cast<int32>(UCoreKitBasicUIScreenRouter::Decide(In)), static_cast<int32>(Expected));
	};

	FCoreKitBasicUIRouterInputs Idle;
	Expect(TEXT("Nothing to do"), Idle, ECoreKitBasicUIRouterAction::None);

	FCoreKitBasicUIRouterInputs Offer;
	Offer.bOfferOpen = true;
	Offer.OfferCards = 3;
	Offer.bPaused = true;
	Expect(TEXT("Offer opens Card Draw (not Pause)"), Offer, ECoreKitBasicUIRouterAction::OpenCardDraw);
	Offer.bCardDrawOpen = true;
	Expect(TEXT("Chained offer stays in the open Card Draw"), Offer, ECoreKitBasicUIRouterAction::None);
	FCoreKitBasicUIRouterInputs EmptyOffer = Offer;
	EmptyOffer.bCardDrawOpen = false;
	EmptyOffer.OfferCards = 0;
	EmptyOffer.PauseHeldFrames = 5;
	Expect(TEXT("Offer without cards does not open Card Draw"), EmptyOffer, ECoreKitBasicUIRouterAction::OpenPause);

	FCoreKitBasicUIRouterInputs Closing;
	Closing.bCardDrawOpen = true;
	Closing.bPaused = true;
	Closing.PauseHeldFrames = 5;
	Expect(TEXT("Card Draw finishing: no Pause behind it"), Closing, ECoreKitBasicUIRouterAction::None);

	FCoreKitBasicUIRouterInputs Paused;
	Paused.bPaused = true;
	Paused.PauseHeldFrames = 1;
	Expect(TEXT("Pause debounced on the first frame"), Paused, ECoreKitBasicUIRouterAction::None);
	Paused.PauseHeldFrames = 2;
	Expect(TEXT("Pause opens"), Paused, ECoreKitBasicUIRouterAction::OpenPause);
	Paused.bPauseOpen = true;
	Expect(TEXT("Pause stays"), Paused, ECoreKitBasicUIRouterAction::None);
	Paused.bPaused = false;
	Expect(TEXT("Resume closes Pause"), Paused, ECoreKitBasicUIRouterAction::ClosePause);
	Paused.bPauseClosing = true;
	Expect(TEXT("Pause already closing"), Paused, ECoreKitBasicUIRouterAction::None);

	FCoreKitBasicUIRouterInputs Modal;
	Modal.bModalOpen = true;
	Modal.bOfferOpen = true;
	Modal.OfferCards = 3;
	Expect(TEXT("Nothing opens over a modal"), Modal, ECoreKitBasicUIRouterAction::None);
	Modal.bOfferOpen = false;
	Modal.bPauseOpen = true;
	Modal.bPaused = false;
	Expect(TEXT("Pause is not closed under a modal"), Modal, ECoreKitBasicUIRouterAction::None);

	FCoreKitBasicUIRouterInputs Defeat;
	Defeat.Phase = ECoreKitBasicUIRunPhase::Defeat;
	Defeat.bPaused = true;
	Defeat.bPauseOpen = true;
	Expect(TEXT("Run end closes run menus first"), Defeat, ECoreKitBasicUIRouterAction::CloseRunMenus);
	Defeat.bPauseOpen = false;
	Expect(TEXT("Then opens Defeat"), Defeat, ECoreKitBasicUIRouterAction::OpenDefeat);
	Defeat.bEndOpen = true;
	Defeat.bEndIsVictory = false;
	Expect(TEXT("Defeat stays"), Defeat, ECoreKitBasicUIRouterAction::None);

	FCoreKitBasicUIRouterInputs Victory;
	Victory.Phase = ECoreKitBasicUIRunPhase::Victory;
	Expect(TEXT("Victory opens"), Victory, ECoreKitBasicUIRouterAction::OpenVictory);
	Victory.bEndOpen = true;
	Victory.bEndIsVictory = false;
	Expect(TEXT("Wrong end screen closes"), Victory, ECoreKitBasicUIRouterAction::CloseEnd);

	FCoreKitBasicUIRouterInputs Retry;
	Retry.bEndOpen = true;
	Expect(TEXT("Retry in place closes the end screen"), Retry, ECoreKitBasicUIRouterAction::CloseEnd);
	return true;
}

#endif
