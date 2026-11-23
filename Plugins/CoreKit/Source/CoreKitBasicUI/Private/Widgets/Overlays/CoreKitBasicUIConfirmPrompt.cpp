#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIConfirmPrompt.h"
#include "CoreKitBasicUI/Settings/CoreKitBasicUISettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CoreKitNavigation/CoreKitButton.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIUserWidget.h"
#include "CoreKitBasicUI/Widgets/Common/CoreKitBasicUIWidgetBuild.h"
#include "CoreKitBasicUI/Widgets/Overlays/CoreKitBasicUIOverlayCommon.h"

#define LOCTEXT_NAMESPACE "CoreKitBasicUIConfirmPrompt"

namespace
{
	const FName KeyName(TEXT("PromptKey"));
	const FName LabelName(TEXT("PromptLabel"));
	const FName InsetName(TEXT("PromptInset"));

	template <typename T>
	T* FindNamedChild(UWidget* Root, FName Base)
	{
		T* Found = nullptr;
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount() && !Found; ++Index)
			{
				UWidget* Child = Panel->GetChildAt(Index);
				if (Child && Child->GetFName().ToString().StartsWith(Base.ToString()))
				{
					Found = Cast<T>(Child);
				}
				if (!Found)
				{
					Found = FindNamedChild<T>(Child, Base);
				}
			}
		}
		return Found;
	}

	bool UsingGamepad(const UUserWidget* Widget)
	{
		const ULocalPlayer* Player = Widget ? Widget->GetOwningLocalPlayer() : nullptr;
		const UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
		return Nav && Nav->GetInputDevice() == ECoreKitInputDevice::Gamepad;
	}
}

UCoreKitBasicUIConfirmPrompt::UCoreKitBasicUIConfirmPrompt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = ECoreKitMenuInputMode::Menu;
	bShowCursor = true;
	bAllowReturn = true;
	bAllowSections = false;
}

TSharedRef<SWidget> UCoreKitBasicUIConfirmPrompt::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UWidgetTree* Tree = WidgetTree;
		UOverlay* Root = CoreKitBasicUIBuild::Overlay(Tree, TEXT("PromptRoot"));
		Tree->RootWidget = Root;
		Veil = CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::SootA(0.72f)), TEXT("Veil"));
		CoreKitBasicUIBuild::AddO(Root, Veil);

		UOverlay* Stack = CoreKitBasicUIBuild::Overlay(Tree, TEXT("DialogStack"));
		USizeBox* DialogBox = CoreKitBasicUIBuild::Box(Tree, Stack, 680.f, 0.f, TEXT("Dialog"));
		Dialog = DialogBox;
		CoreKitBasicUIBuild::AddO(Root, DialogBox, HAlign_Center, VAlign_Center);
		CoreKitBasicUIBuild::AddO(Stack, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Chamfer(22, CoreKitBasicUIColor::Hull), TEXT("DialogPlate")));

		UVerticalBox* Column = CoreKitBasicUIBuild::Column(Tree, TEXT("DialogColumn"));
		CoreKitBasicUIBuild::AddO(Stack, Column);

		// 12 px magenta/Hull tape (14 px stripes), clipped by the dialog's 22 px chamfer.
		UCanvasPanel* TapeCanvas = CoreKitBasicUIBuild::Canvas(Tree, TEXT("TapeCanvas"));
		CoreKitBasicUIBuild::AddV(Column, CoreKitBasicUIBuild::Box(Tree, TapeCanvas, 680.f, 12.f));
		UCanvasPanel* TapeClip = CoreKitBasicUIOverlay::AddChamferClip(Tree, TapeCanvas, FVector2D::ZeroVector, FVector2D(680.f, 12.f), 22.f, TEXT("TapeClip"));
		CoreKitBasicUIBuild::Place(TapeClip, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Solid(CoreKitBasicUIColor::Hull)), FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(680.f, 12.f));
		CoreKitBasicUIBuild::Place(TapeClip, CoreKitBasicUIBuild::Image(Tree, UCoreKitBasicUIStyle::Tape(14, CoreKitBasicUIColor::Magenta)), FAnchors(0.f, 0.f), FVector2D(-20.f, -20.f), FVector2D(720.f, 52.f));

		UVerticalBox* Content = CoreKitBasicUIBuild::Column(Tree, TEXT("DialogContent"));
		CoreKitBasicUIBuild::AddV(Column, Content, FMargin(44.f, 36.f, 44.f, 40.f));
		CoreKitBasicUIBuild::AddV(Content, CoreKitBasicUIBuild::Text(Tree, COREKIT_BASICUI_TEXT("Confirm", "CONFIRM"), CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(13.f), 500, CoreKitBasicUIColor::Magenta, 0.24f), TEXT("Kicker")));
		Title = CoreKitBasicUIBuild::Text(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(52.f), 700), TEXT("Title"));
		Title->SetAutoWrapText(true);
		CoreKitBasicUIBuild::AddV(Content, Title, FMargin(0.f, 16.f, 0.f, 0.f));
		Body = CoreKitBasicUIBuild::Text(Tree, FText::GetEmpty(), CoreKitBasicUIBuild::Body(CoreKitBasicUIBuild::Px(24.f), 400, CoreKitBasicUIColor::TextDim), TEXT("Body"));
		Body->SetAutoWrapText(true);
		CoreKitBasicUIBuild::AddV(Content, Body, FMargin(0.f, 16.f, 0.f, 0.f));
		UHorizontalBox* OptionRow = CoreKitBasicUIBuild::Row(Tree, TEXT("OptionPanel"));
		OptionPanel = OptionRow;
		CoreKitBasicUIBuild::AddV(Content, OptionRow, FMargin(0.f, 30.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UCoreKitBasicUIConfirmPrompt::NativeConstruct()
{
	Super::NativeConstruct();
	OnPromptDecided.AddUniqueDynamic(this, &ThisClass::HandleDecided);
	Appear.Start(0.f, 1.f, ECoreKitBasicUIMotion::Settle);
	CoreKitBasicUISound::Play(this, TEXT("UI.Open"));
	if (!bLowPass)
	{
		CoreKitBasicUIOverlay::PushLowPass(this);
		bLowPass = true;
	}
}

void UCoreKitBasicUIConfirmPrompt::NativeDestruct()
{
	if (bLowPass)
	{
		CoreKitBasicUIOverlay::PopLowPass(this);
		bLowPass = false;
	}
	Super::NativeDestruct();
}

void UCoreKitBasicUIConfirmPrompt::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	Appear.Tick(FMath::Min(DeltaTime, 0.1f));
	const float A = Appear.Value;
	if (Veil)
	{
		Veil->SetRenderOpacity(A);
	}
	if (Dialog)
	{
		CoreKitBasicUIOverlay::Pose(Dialog, FVector2D(0.f, (1.f - A) * 12.f), 0.97f + 0.03f * A, 0.f, A);
	}
}

void UCoreKitBasicUIConfirmPrompt::NativeOnPromptSetup()
{
	if (Title)
	{
		Title->SetText(PromptTitle.ToUpper());
	}
	const bool bPad = UsingGamepad(this);
	for (int32 Index = 0; Index < OptionButtons.Num(); ++Index)
	{
		UCoreKitButton* Button = OptionButtons[Index];
		if (!Button)
		{
			continue;
		}
		const bool bCancel = Index == CancelOption;
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Button->Slot))
		{
			FSlateChildSize Fill(ESlateSizeRule::Fill);
			Fill.Value = 1.f;
			BoxSlot->SetSize(Fill);
			BoxSlot->SetPadding(FMargin(Index > 0 ? 14.f : 0.f, 0.f, 0.f, 0.f));
		}
		const FLinearColor Normal = bCancel ? CoreKitBasicUIColor::Plate : (bDanger ? CoreKitBasicUIColor::Red : CoreKitBasicUIColor::Magenta);
		const FLinearColor Selected = bCancel ? CoreKitBasicUIColor::White : Normal;
		Button->SetBaseStyle(UCoreKitBasicUIStyle::ButtonStyle(16, Normal, Selected, Selected));

		Button->ClearChildren();
		UOverlay* Face = CoreKitBasicUIBuild::Overlay(WidgetTree);
		UImage* Inset = CoreKitBasicUIBuild::Image(WidgetTree, UCoreKitBasicUIStyle::Solid(bCancel ? CoreKitBasicUIColor::Magenta : CoreKitBasicUIColor::White),
			MakeUniqueObjectName(WidgetTree, UImage::StaticClass(), InsetName));
		Inset->SetVisibility(ESlateVisibility::Collapsed);
		CoreKitBasicUIBuild::AddO(Face, CoreKitBasicUIBuild::Box(WidgetTree, Inset, 8.f, 0.f), HAlign_Left, VAlign_Fill);
		UHorizontalBox* Row = CoreKitBasicUIBuild::Row(WidgetTree);
		CoreKitBasicUIBuild::AddO(Face, Row, HAlign_Center, VAlign_Center);
		const FText KeyText = bCancel ? (bPad ? COREKIT_BASICUI_TEXT("PadB", "B") : COREKIT_BASICUI_TEXT("Esc", "ESC")) : (bPad ? COREKIT_BASICUI_TEXT("PadA", "A") : COREKIT_BASICUI_TEXT("Enter", "ENTER"));
		UTextBlock* Key = CoreKitBasicUIBuild::Text(WidgetTree, KeyText, CoreKitBasicUIBuild::Mono(CoreKitBasicUIBuild::Px(12.f), 700, bCancel ? CoreKitBasicUIColor::Steel : CoreKitBasicUIColor::Soot, 0.14f),
			MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), KeyName));
		CoreKitBasicUIBuild::AddH(Row, Key, FMargin(0.f, 0.f, 14.f, 0.f));
		const FText LabelText = Options.IsValidIndex(Index) ? Options[Index].ToUpper() : FText::GetEmpty();
		UTextBlock* Label = CoreKitBasicUIBuild::Text(WidgetTree, LabelText, CoreKitBasicUIBuild::Display(CoreKitBasicUIBuild::Px(26.f), 700, bCancel ? CoreKitBasicUIColor::White : CoreKitBasicUIColor::Soot),
			MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), LabelName));
		CoreKitBasicUIBuild::AddH(Row, Label);
		Button->AddChild(CoreKitBasicUIBuild::Box(WidgetTree, Face, 0.f, 64.f));
		if (UButtonSlot* Content = Cast<UButtonSlot>(Button->GetContentSlot()))
		{
			Content->SetPadding(FMargin(0.f));
			Content->SetHorizontalAlignment(HAlign_Fill);
			Content->SetVerticalAlignment(VAlign_Fill);
		}
		Button->OnSelected.AddUniqueDynamic(this, &ThisClass::HandleButtonSelected);
		Button->OnDeselected.AddUniqueDynamic(this, &ThisClass::HandleButtonDeselected);
		StyleButton(Button, Button->IsSelected());
	}
}

void UCoreKitBasicUIConfirmPrompt::StyleButton(UCoreKitButton* Button, bool bSelected)
{
	if (!Button)
	{
		return;
	}
	const bool bCancel = Button->ButtonIndex == CancelOption;
	UWidget* Content = Button->GetChildrenCount() > 0 ? Button->GetChildAt(0) : nullptr;
	if (UImage* Inset = FindNamedChild<UImage>(Content, InsetName))
	{
		Inset->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (UTextBlock* Label = FindNamedChild<UTextBlock>(Content, LabelName))
	{
		Label->SetColorAndOpacity(FSlateColor(bCancel && !bSelected ? CoreKitBasicUIColor::White : CoreKitBasicUIColor::Soot));
	}
	if (UTextBlock* Key = FindNamedChild<UTextBlock>(Content, KeyName))
	{
		Key->SetColorAndOpacity(FSlateColor(bCancel && !bSelected ? CoreKitBasicUIColor::Steel : CoreKitBasicUIColor::Soot));
	}
}

void UCoreKitBasicUIConfirmPrompt::HandleButtonSelected(UCoreKitButton* Button)
{
	StyleButton(Button, true);
}

void UCoreKitBasicUIConfirmPrompt::HandleButtonDeselected(UCoreKitButton* Button)
{
	StyleButton(Button, false);
}

void UCoreKitBasicUIConfirmPrompt::HandleDecided(int32 Option)
{
	TFunction<void(bool)> Pending = MoveTemp(OnConfirmed);
	OnConfirmed = nullptr;
	if (Pending)
	{
		Pending(Option == ConfirmIndex);
	}
}

void CoreKitBasicUIPrompt::PresetTexts(ECoreKitBasicUIPromptPreset Preset, float RunSeconds, int32 Level, int32 Cards, FText& OutTitle, FText& OutBody, FText& OutConfirm)
{
	OutBody = FText::GetEmpty();
	switch (Preset)
	{
	case ECoreKitBasicUIPromptPreset::RestartRun:
		OutTitle = COREKIT_BASICUI_TEXT("RestartTitle", "Restart run?");
		OutBody = CoreKitBasicUIOverlay::RestartBody(RunSeconds);
		OutConfirm = COREKIT_BASICUI_TEXT("RestartAction", "RESTART");
		break;
	case ECoreKitBasicUIPromptPreset::QuitToMenu:
		OutTitle = COREKIT_BASICUI_TEXT("QuitMenuTitle", "Quit to menu?");
		OutBody = CoreKitBasicUIOverlay::QuitToMenuBody(RunSeconds, Level, Cards);
		OutConfirm = COREKIT_BASICUI_TEXT("QuitAction", "QUIT");
		break;
	case ECoreKitBasicUIPromptPreset::QuitToDesktop:
		// Body pending (Docs/UI/Spec/11-Decisions.md § Pendientes).
		OutTitle = COREKIT_BASICUI_TEXT("QuitDesktopTitle", "Quit to desktop?");
		OutConfirm = COREKIT_BASICUI_TEXT("QuitDesktopAction", "QUIT");
		break;
	case ECoreKitBasicUIPromptPreset::ResetDefaults:
		OutTitle = COREKIT_BASICUI_TEXT("ResetTitle", "Reset to defaults?");
		OutConfirm = COREKIT_BASICUI_TEXT("ResetAction", "RESET");
		break;
	case ECoreKitBasicUIPromptPreset::DiscardChanges:
		OutTitle = COREKIT_BASICUI_TEXT("DiscardTitle", "Discard changes?");
		OutConfirm = COREKIT_BASICUI_TEXT("DiscardAction", "DISCARD");
		break;
	}
}

UCoreKitBasicUIConfirmPrompt* CoreKitBasicUIPrompt::Show(APlayerController* Controller, const FText& InTitle, const FText& InBody, const FText& ConfirmLabel,
	TFunction<void(bool)> OnDecided, bool bDanger)
{
	ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	UCoreKitNavigationSubsystem* Nav = Player ? Player->GetSubsystem<UCoreKitNavigationSubsystem>() : nullptr;
	if (!Nav)
	{
		return nullptr;
	}
	UCoreKitBasicUIConfirmPrompt* Prompt = CreateWidget<UCoreKitBasicUIConfirmPrompt>(Controller, UCoreKitBasicUIConfirmPrompt::StaticClass());
	if (!Prompt)
	{
		return nullptr;
	}
	Prompt->SetDanger(bDanger);
	Prompt->OnConfirmed = MoveTemp(OnDecided);
	Prompt->SetupPrompt(InTitle, InBody, { COREKIT_BASICUI_TEXT("Cancel", "CANCEL"), ConfirmLabel }, UCoreKitBasicUIConfirmPrompt::CancelIndex, UCoreKitBasicUIConfirmPrompt::ConfirmIndex);
	Nav->PushMenu(Prompt, TEXT("Modal"));
	return Prompt;
}

UCoreKitBasicUIConfirmPrompt* CoreKitBasicUIPrompt::ShowPreset(APlayerController* Controller, ECoreKitBasicUIPromptPreset Preset, TFunction<void(bool)> OnDecided, float RunSeconds, int32 Level, int32 Cards)
{
	FText PresetTitle, PresetBody, PresetConfirm;
	PresetTexts(Preset, RunSeconds, Level, Cards, PresetTitle, PresetBody, PresetConfirm);
	return Show(Controller, PresetTitle, PresetBody, PresetConfirm, MoveTemp(OnDecided), true);
}

#undef LOCTEXT_NAMESPACE
