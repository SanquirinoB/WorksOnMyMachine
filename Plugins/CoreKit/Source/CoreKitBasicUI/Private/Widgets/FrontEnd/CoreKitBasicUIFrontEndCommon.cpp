#include "Widgets/FrontEnd/CoreKitBasicUIFrontEndCommon.h"

#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/EngineVersion.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIFrontEnd"

// ---------------------------------------------------------------------------------------------------------------
// Device keycap

void UCoreKitBasicUIDeviceKeycap::SetKeys(FKey InKeyboardKey, FKey InGamepadKey)
{
	KeyboardKey = InKeyboardKey;
	GamepadKey = InGamepadKey;
	SetKey(WantedKey());
}

FKey UCoreKitBasicUIDeviceKeycap::WantedKey() const
{
	const ULocalPlayer* Player = GetOwningLocalPlayer();
	const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	const bool bPad = Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad;
	return bPad && GamepadKey.IsValid() ? GamepadKey : KeyboardKey;
}

void UCoreKitBasicUIDeviceKeycap::NativeConstruct()
{
	Super::NativeConstruct();
	if (KeyboardKey.IsValid() || GamepadKey.IsValid())
	{
		SetKey(WantedKey());
	}
}

void UCoreKitBasicUIDeviceKeycap::NativeOnPresentationChanged(const FCoreKitKeyPresentation& NewPresentation, bool bHasKey)
{
	// The prompt refreshes on device changes; switch the fixed key to the new device family first.
	const FKey Wanted = WantedKey();
	if (!bSwapping && Wanted.IsValid() && Key != Wanted)
	{
		TGuardValue<bool> Guard(bSwapping, true);
		SetKey(Wanted);
		return;
	}
	Super::NativeOnPresentationChanged(NewPresentation, bHasKey);
	if (bHiddenOnGamepad)
	{
		const ULocalPlayer* Player = GetOwningLocalPlayer();
		const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
		const ESlateVisibility Shown = Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
		SetVisibility(Shown);
		if (UWidget* Parent = GetParent(); Parent && Parent->IsA<USizeBox>())
		{
			Parent->SetVisibility(Shown);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Layout helpers

namespace CoreKitBasicUIFrontEndUI
{
	UCanvasPanel* MakeScreenRoot(UWidgetTree* Tree, bool bOpaque)
	{
		UCanvasPanel* Root = CoreKitBasicUIBuild::Canvas(Tree, TEXT("Root"));
		Tree->RootWidget = Root;
		if (bOpaque)
		{
			CoreKitBasicUIBuild::Fill(Root, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot), TEXT("Background")));
			CoreKitBasicUIBuild::Fill(Root, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Scanlines(0.025f), TEXT("Scanlines")));
		}
		return Root;
	}

	UWidget* MakeTitleBlock(UWidgetTree* Tree, const FText& Kicker, const FText& Title)
	{
		UVerticalBox* Block = CoreKitBasicUIBuild::Column(Tree, TEXT("TitleBlock"));
		CoreKitBasicUIBuild::AddV(Block, CoreKitBasicUIBuild::Text(Tree, Kicker, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), 500, CoreKitBasicUIColor::Magenta, 0.24f), TEXT("Kicker")));
		CoreKitBasicUIBuild::FTextStyle TitleStyle = CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(84.f), 700);
		TitleStyle.bUpper = true;
		CoreKitBasicUIBuild::AddV(Block, CoreKitBasicUIBuild::Text(Tree, Title, TitleStyle, TEXT("Title")), FMargin(0.f, -4.f, 0.f, 0.f));
		return Block;
	}

	UCoreKitBasicUIDeviceKeycap* MakeKeycap(UWidgetTree* Tree, FKey KeyboardKey, FKey GamepadKey, const FLinearColor& Accent, float Height)
	{
		UCoreKitBasicUIDeviceKeycap* Keycap = Tree->ConstructWidget<UCoreKitBasicUIDeviceKeycap>(UCoreKitBasicUIDeviceKeycap::StaticClass());
		Keycap->SetAccent(Accent);
		Keycap->SetKeys(KeyboardKey, GamepadKey);
		return Keycap;
	}

	UWidget* MakeKeyHint(UWidgetTree* Tree, FKey KeyboardKey, FKey GamepadKey, const FText& Label, const FLinearColor& LabelColor,
		const FLinearColor& Accent, UCoreKitButton** OutButton, UTextBlock** OutLabel, UCoreKitBasicUIDeviceKeycap** OutKeycap)
	{
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(Tree);
		UCoreKitBasicUIDeviceKeycap* Keycap = MakeKeycap(Tree, KeyboardKey, GamepadKey, Accent);
		USizeBox* KeyBox = CoreKitBasicUIBuild::Box(Tree, Keycap, 0.f, 38.f);
		KeyBox->SetMinDesiredWidth(38.f);
		CoreKitBasicUIBuild::AddH(Row, KeyBox);
		UTextBlock* Text = CoreKitBasicUIBuild::Text(Tree, Label, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(14.f), 500, LabelColor, 0.14f));
		CoreKitBasicUIBuild::AddH(Row, Text, FMargin(12.f, 0.f, 0.f, 0.f));
		if (OutLabel)
		{
			*OutLabel = Text;
		}
		if (OutKeycap)
		{
			*OutKeycap = Keycap;
		}
		if (OutButton)
		{
			*OutButton = MakeClearButton(Tree, Row, false);
			return *OutButton;
		}
		return Row;
	}

	UTextBlock* MakeHeading(UWidgetTree* Tree, const FText& Text)
	{
		CoreKitBasicUIBuild::FTextStyle Style = CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Muted, 0.22f);
		Style.bUpper = true;
		return CoreKitBasicUIBuild::Text(Tree, Text, Style);
	}

	UTextBlock* MakeParagraph(UWidgetTree* Tree, const FText& Text, const CoreKitBasicUIBuild::FTextStyle& Style, float WrapAt)
	{
		UTextBlock* Block = CoreKitBasicUIBuild::Text(Tree, Text, Style);
		Block->SetAutoWrapText(WrapAt <= 0.f);
		if (WrapAt > 0.f)
		{
			Block->SetWrapTextAt(WrapAt);
		}
		return Block;
	}

	URichTextBlock* MakeRich(UWidgetTree* Tree, const FText& Markup, const CoreKitBasicUIBuild::FTextStyle& Base)
	{
		UDataTable* Styles = NewObject<UDataTable>(Tree, NAME_None, RF_Transient);
		Styles->RowStruct = FRichTextStyleRow::StaticStruct();
		auto AddStyle = [Styles, &Base](FName Name, const FLinearColor& Color, ECoreKitBasicUIFontFamily Family, int32 Weight)
		{
			FRichTextStyleRow Row;
			Row.TextStyle.SetFont(UCoreKitBasicUIStyle::Font(Family, Weight, Base.Size, Base.Tracking));
			Row.TextStyle.SetColorAndOpacity(FSlateColor(Color));
			Styles->AddRow(Name, Row);
		};
		AddStyle(TEXT("Default"), Base.Color, Base.Family, Base.Weight);
		AddStyle(TEXT("m"), CoreKitBasicUIColor::Magenta, ECoreKitBasicUIFontFamily::Display, 700);
		AddStyle(TEXT("o"), CoreKitBasicUIColor::Orange, Base.Family, 700);
		AddStyle(TEXT("l"), CoreKitBasicUIColor::Lime, Base.Family, 700);
		AddStyle(TEXT("t"), CoreKitBasicUIColor::Mint, Base.Family, 700);
		AddStyle(TEXT("w"), CoreKitBasicUIColor::White, Base.Family, Base.Weight);
		AddStyle(TEXT("b"), CoreKitBasicUIColor::White, Base.Family, 700);
		URichTextBlock* Rich = Tree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass());
		Rich->SetTextStyleSet(Styles);
		Rich->SetAutoWrapText(true);
		Rich->SetText(Markup);
		return Rich;
	}

	UWidget* MakeRule(UWidgetTree* Tree, const FLinearColor& Color, float Thickness)
	{
		return CoreKitBasicUIBuild::Box(Tree, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(Color)), 0.f, Thickness);
	}

	UCoreKitButton* MakeClearButton(UWidgetTree* Tree, UWidget* Content, bool bFocusable, FName Name)
	{
		UCoreKitButton* Button = Tree->ConstructWidget<UCoreKitButton>(UCoreKitButton::StaticClass(), Name);
		Button->SetBaseStyle(UCoreKitBasicUIStyle::ClearButtonStyle());
		Button->SetFocusableBeforeConstruct(bFocusable);
		Button->bSelectOnHover = bFocusable;
		if (Content)
		{
			if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Content)))
			{
				ButtonSlot->SetPadding(FMargin(0.f));
				ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
				ButtonSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		return Button;
	}

	void Confirm(UUserWidget* Owner, ECoreKitBasicUIPromptPreset Preset, const FText& Body, TFunction<void(bool)> OnDecided)
	{
		FText Title, PresetBody, Action;
		CoreKitBasicUIPrompt::PresetTexts(Preset, 0.f, 1, 0, Title, PresetBody, Action);
		// Without a navigation stack (no player) nothing is shown and the action is not confirmed.
		CoreKitBasicUIPrompt::Show(Owner ? Owner->GetOwningPlayer() : nullptr, Title, Body.IsEmpty() ? PresetBody : Body, Action, MoveTemp(OnDecided), true);
	}

	float Step(float Time, float Start, float Duration, ECoreKitBasicUIMotion Curve)
	{
		if (Time < Start)
		{
			return 0.f;
		}
		if (Duration <= 0.f || Time >= Start + Duration)
		{
			return 1.f;
		}
		return CoreKitBasicUIMotion::Ease(Curve, (Time - Start) / Duration);
	}

	void Enter(UWidget* Widget, float Alpha, FVector2D From)
	{
		if (!Widget)
		{
			return;
		}
		Widget->SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
		const FVector2D Offset = CoreKitBasicUIMotion::IsReducedMotion() ? FVector2D::ZeroVector : From * (1.f - Alpha);
		FWidgetTransform Transform = Widget->GetRenderTransform();
		Transform.Translation = Offset;
		Widget->SetRenderTransform(Transform);
	}

	void Wipe(const UUserWidget* Widget)
	{
		if (Widget)
		{
			UCoreKitBasicUIScreenWipe::Play(Widget->GetOwningPlayer(), ECoreKitBasicUIWipeMode::Reveal);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Screen wipe

float UCoreKitBasicUIScreenWipe::GetDuration()
{
	return GetDefault<UCoreKitBasicUIMotionSettings>()->ScreenWipe;
}

UCoreKitBasicUIScreenWipe* UCoreKitBasicUIScreenWipe::Play(APlayerController* Controller, ECoreKitBasicUIWipeMode InMode, TFunction<void()> OnDone)
{
	UCoreKitBasicUIScreenWipe* Wipe = Controller && Controller->IsLocalController() ? CreateWidget<UCoreKitBasicUIScreenWipe>(Controller, UCoreKitBasicUIScreenWipe::StaticClass()) : nullptr;
	if (!Wipe)
	{
		if (OnDone)
		{
			OnDone();
		}
		return nullptr;
	}
	Wipe->Mode = InMode;
	Wipe->Done = MoveTemp(OnDone);
	Wipe->SetVisibility(ESlateVisibility::HitTestInvisible);
	Wipe->AddToPlayerScreen(20000);
	Wipe->Layout(0.f);
	return Wipe;
}

void UCoreKitBasicUIScreenWipe::BuildTree()
{
	Root = CoreKitBasicUIBuild::Canvas(WidgetTree, TEXT("Root"));
	WidgetTree->RootWidget = Root;
	Cover = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Soot), TEXT("Cover"));
	Glow = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.75f)), TEXT("Glow"));
	Line = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta), TEXT("Line"));
	Core = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::WhiteA(0.7f)), TEXT("Core"));
	for (UImage* Part : { Cover.Get(), Glow.Get(), Line.Get(), Core.Get() })
	{
		CoreKitBasicUIBuild::Place(Root, Part, FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(10.f, 10.f));
	}
}

void UCoreKitBasicUIScreenWipe::Layout(float Alpha)
{
	if (!Root)
	{
		return;
	}
	FVector2D Size = GetCachedGeometry().GetLocalSize();
	if (Size.X < 2.f || Size.Y < 2.f)
	{
		const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
		Size = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	}
	if (Size.X < 2.f || Size.Y < 2.f)
	{
		Size = FVector2D(1920.f, 1080.f);
	}
	constexpr float Angle = 12.f;
	const float Lean = Size.Y * FMath::Tan(FMath::DegreesToRadians(Angle));
	const float Margin = Lean + 80.f;
	const float X = FMath::Lerp(-Margin, Size.X + Margin, CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Deal, Alpha));
	const float Tall = Size.Y * 1.8f;
	const FVector2D Mid(X, Size.Y * 0.5f);

	auto Set = [Angle](UImage* Image, FVector2D Position, FVector2D ImageSize, FVector2D Alignment)
	{
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Image->Slot))
		{
			PanelSlot->SetPosition(Position);
			PanelSlot->SetSize(ImageSize);
			PanelSlot->SetAlignment(Alignment);
		}
		Image->SetRenderTransformPivot(Alignment);
		Image->SetRenderTransformAngle(Angle);
	};
	const bool bReveal = Mode == ECoreKitBasicUIWipeMode::Reveal;
	Set(Cover, Mid, FVector2D(Size.X * 2.f + Margin * 2.f, Tall), FVector2D(bReveal ? 0.f : 1.f, 0.5f));
	Set(Glow, Mid, FVector2D(72.f, Tall), FVector2D(0.5f, 0.5f));
	Set(Line, Mid, FVector2D(8.f, Tall), FVector2D(0.5f, 0.5f));
	Set(Core, Mid, FVector2D(2.f, Tall), FVector2D(0.5f, 0.5f));
}

void UCoreKitBasicUIScreenWipe::TickMotion(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	const float Duration = FMath::Max(GetDuration(), 0.01f);
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	Layout(Alpha);
	if (Alpha >= 1.f)
	{
		bFinished = true;
		TFunction<void()> Callback = MoveTemp(Done);
		if (Mode == ECoreKitBasicUIWipeMode::Reveal)
		{
			RemoveFromParent();
		}
		else
		{
			Glow->SetVisibility(ESlateVisibility::Collapsed);
			Line->SetVisibility(ESlateVisibility::Collapsed);
			Core->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Callback)
		{
			Callback();
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Loading screen

void UCoreKitBasicUILoadingScreen::BuildTree()
{
	UCanvasPanel* Root = CoreKitBasicUIFrontEndUI::MakeScreenRoot(WidgetTree);

	// The slash a Cover wipe ends on, parked on the right edge.
	UOverlay* Slash = CoreKitBasicUIBuild::Overlay(WidgetTree, TEXT("Slash"));
	CoreKitBasicUIBuild::AddO(Slash, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Image(TEXT("T_UI_Glow"), CoreKitBasicUIColor::WithAlpha(CoreKitBasicUIColor::Magenta, 0.6f))), 72.f, 0.f), HAlign_Center);
	CoreKitBasicUIBuild::AddO(Slash, CoreKitBasicUIBuild::Box(WidgetTree, CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta)), 8.f, 0.f), HAlign_Center);
	UCanvasPanelSlot* SlashSlot = CoreKitBasicUIBuild::Place(Root, Slash, FAnchors(1.f, 0.f, 1.f, 1.f), FVector2D(-70.f, -200.f), FVector2D(72.f, 0.f), FVector2D(0.5f, 0.f));
	SlashSlot->SetOffsets(FMargin(-70.f, -200.f, 72.f, -200.f));
	Slash->SetRenderTransformAngle(12.f);

	// Small logo plates, bottom left, with the status line.
	UVerticalBox* Block = CoreKitBasicUIBuild::Column(WidgetTree, TEXT("Block"));
	UHorizontalBox* Plates = CoreKitBasicUIBuild::Row(WidgetTree);
	UBorder* Dead = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::Chamfer(10, CoreKitBasicUIColor::Magenta), FMargin(14.f, 6.f, 12.f, 2.f));
	Dead->SetContent(CoreKitBasicUIBuild::Text(WidgetTree, GetDefault<UCoreKitBasicUISettings>()->TitleFirst, CoreKitBasicUIBuild::Stencil(CoreKitBasicUIBuild::Px(44.f), CoreKitBasicUIColor::Soot)));
	UBorder* Draw = CoreKitBasicUIBuild::Panel(WidgetTree, UCoreKitBasicUIStyle::ChamferBottomRight(CoreKitBasicUIColor::Plate), FMargin(12.f, 6.f, 14.f, 2.f));
	Draw->SetContent(CoreKitBasicUIBuild::Text(WidgetTree, GetDefault<UCoreKitBasicUISettings>()->TitleSecond, CoreKitBasicUIBuild::Stencil(CoreKitBasicUIBuild::Px(44.f))));
	CoreKitBasicUIBuild::AddH(Plates, Dead);
	CoreKitBasicUIBuild::AddH(Plates, Draw);
	CoreKitBasicUIBuild::AddV(Block, Plates, FMargin(0.f), HAlign_Left);
	CoreKitBasicUIBuild::AddV(Block, CoreKitBasicUIBuild::Text(WidgetTree, GetDefault<UCoreKitBasicUISettings>()->LoadingLabel, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::TextDim, 0.3f)), FMargin(0.f, 18.f, 0.f, 0.f));
	UImage* Bar = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Magenta), TEXT("Pulse"));
	CoreKitBasicUIBuild::AddV(Block, CoreKitBasicUIBuild::Box(WidgetTree, Bar, 160.f, 4.f), FMargin(0.f, 12.f, 0.f, 0.f), HAlign_Left);
	Pulse = Bar;
	CoreKitBasicUIBuild::Place(Root, Block, FAnchors(0.f, 1.f), FVector2D(120.f, -96.f), FVector2D::ZeroVector, FVector2D(0.f, 1.f));
}

void UCoreKitBasicUILoadingScreen::TickMotion(float DeltaSeconds)
{
	Time += DeltaSeconds;
	if (Pulse)
	{
		Pulse->SetRenderOpacity(0.35f + 0.65f * CoreKitBasicUIMotion::Ease(ECoreKitBasicUIMotion::Pulse, FMath::Fmod(Time, 1.f)));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Public entry points

namespace
{
	template <typename T>
	T* OpenScreen(APlayerController* Controller)
	{
		ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
		UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
		T* Menu = Controller ? CreateWidget<T>(Controller, T::StaticClass()) : nullptr;
		if (Menu && Nav)
		{
			Nav->PushMenu(Menu, TEXT("Menu"));
			UCoreKitBasicUIScreenWipe::Play(Controller, ECoreKitBasicUIWipeMode::Reveal);
		}
		return Menu;
	}
}

UCoreKitBasicUISettingsMenu* CoreKitBasicUIFrontEnd::OpenSettings(APlayerController* Controller) { return OpenScreen<UCoreKitBasicUISettingsMenu>(Controller); }
UCoreKitBasicUIHowToPlayMenu* CoreKitBasicUIFrontEnd::OpenHowToPlay(APlayerController* Controller) { return OpenScreen<UCoreKitBasicUIHowToPlayMenu>(Controller); }
UCoreKitBasicUICreditsMenu* CoreKitBasicUIFrontEnd::OpenCredits(APlayerController* Controller) { return OpenScreen<UCoreKitBasicUICreditsMenu>(Controller); }

void CoreKitBasicUIFrontEnd::CloseScreen(UCoreKitMenuWidget* Screen)
{
	if (!Screen)
	{
		return;
	}
	UCoreKitBasicUIScreenWipe::Play(Screen->GetOwningPlayer(), ECoreKitBasicUIWipeMode::Reveal);
	Screen->CloseMenu();
}

FText CoreKitBasicUIFrontEnd::GetBuildVersionText()
{
	FString Version;
	if (!GConfig || !GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni) || Version.IsEmpty())
	{
		Version = TEXT("0.1");
	}
	// "1.0.0.0" style versions read better trimmed of trailing zero groups.
	while (Version.EndsWith(TEXT(".0")) && Version.Len() > 3)
	{
		Version.LeftChopInline(2);
	}
	return FText::Format(COREKIT_BASICUI_TEXT("BuildVersion", "BUILD {0} // UNREAL ENGINE {1}"), FText::FromString(Version),
		FText::AsNumber(FEngineVersion::Current().GetMajor()));
}

float CoreKitBasicUIFrontEnd::GetMenuMusicVolume()
{
	return 0.4f;
}

#if !UE_BUILD_SHIPPING
// Review helper: "corekit.basicui.FrontEnd Settings|HowToPlay|Credits [Tab]" opens a front-end screen once the title screen is
// the top menu (works from -ExecCmds before the menu exists). With Settings, an optional tab index 0..3 follows.
static FAutoConsoleCommandWithWorldAndArgs GCoreKitBasicUIFrontEndCommand(
	TEXT("corekit.basicui.FrontEnd"),
	TEXT("Opens a front-end screen over the title screen: corekit.basicui.FrontEnd Settings|HowToPlay|Credits [SettingsTab]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.IsEmpty() || !World)
		{
			return;
		}
		const FString Screen = Args[0];
		const int32 Tab = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		const double Deadline = FPlatformTime::Seconds() + 30.0;
		TWeakObjectPtr<UGameInstance> Game = World->GetGameInstance();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Game, Screen, Tab, Deadline](float)
		{
			UGameInstance* Instance = Game.Get();
			APlayerController* Controller = Instance ? Instance->GetFirstLocalPlayerController(Instance->GetWorld()) : nullptr;
			ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
			UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
			if (!Nav || FPlatformTime::Seconds() > Deadline)
			{
				return Nav == nullptr && FPlatformTime::Seconds() <= Deadline;
			}
			UCoreKitMenuWidget* Top = Nav->GetTopMenu();
			if (!Top || (!Top->IsA<UCoreKitBasicUIMainMenu>() && FPlatformTime::Seconds() < Deadline - 25.0))
			{
				return true;
			}
			if (Screen.Equals(TEXT("Settings"), ESearchCase::IgnoreCase))
			{
				if (UCoreKitBasicUISettingsMenu* Settings = CoreKitBasicUIFrontEnd::OpenSettings(Controller))
				{
					Settings->HandleSection(Tab);
				}
			}
			else if (Screen.Equals(TEXT("HowToPlay"), ESearchCase::IgnoreCase))
			{
				CoreKitBasicUIFrontEnd::OpenHowToPlay(Controller);
			}
			else if (Screen.Equals(TEXT("Credits"), ESearchCase::IgnoreCase))
			{
				CoreKitBasicUIFrontEnd::OpenCredits(Controller);
			}
			return false;
		}), 0.25f);
	}));
#endif

#undef LOCTEXT_NAMESPACE
