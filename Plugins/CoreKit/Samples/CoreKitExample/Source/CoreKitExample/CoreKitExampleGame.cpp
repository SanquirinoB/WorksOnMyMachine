#include "CoreKitExampleGame.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoreKitData/CoreKitDataAssetLibrary.h"
#include "CoreKitData/CoreKitDataLoadRequest.h"
#include "CoreKitData/CoreKitDataContainer.h"
#include "CoreKitExampleActors.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "CoreKitNavigation/CoreKitActionGlyphWidget.h"
#include "CoreKitNavigation/CoreKitNavigationGlyphs.h"
#include "CoreKitNavigation/CoreKitNavigationSubsystem.h"
#include "CoreKitTween/CoreKitTweenSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, CoreKitExample, "CoreKitExample");

namespace
{
UTextBlock* MakeText(UWidgetTree& Tree, const FText& Text, int32 Size)
{
	auto* Label = Tree.ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = Size;
	Label->SetFont(Font);
	Label->SetText(Text);
	Label->SetAutoWrapText(true);
	return Label;
}

ACoreKitExampleDoor* FindDoor(UWorld* World)
{
	if (World)
	{
		for (TActorIterator<ACoreKitExampleDoor> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

}

TSharedRef<SWidget> UCoreKitExampleLoadingWidget::RebuildWidget()
{
	return SNew(SBorder).BorderBackgroundColor(FLinearColor(.015f, .025f, .05f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(STextBlock).Text(INVTEXT("Preparing the playground..."))];
}

TSharedRef<SWidget> UCoreKitExamplePanel::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		auto* Game = GetGameInstance<UCoreKitExampleGame>();
		auto* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetPadding(FMargin(24.0f));
		Background->SetHorizontalAlignment(HAlign_Left);
		Background->SetVerticalAlignment(VAlign_Top);
		Background->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Background->SetBrushColor(FLinearColor(.04f, .07f, .13f, .3f));
		WidgetTree->RootWidget = Background;
		auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Background->SetContent(Column);
		Column->AddChildToVerticalBox(MakeText(*WidgetTree, INVTEXT("COREKIT  /  PLAYGROUND"), 25))->SetPadding(FMargin(0, 0, 0, 16));

		auto AddButton = [this, Column, Game](const FText& Title, FName Handler)
		{
			auto* Button = WidgetTree->ConstructWidget<UButton>();
			Button->SetContent(MakeText(*WidgetTree, Title, 16));
			Column->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 3));
			FScriptDelegate Delegate;
			Delegate.BindUFunction(Game, Handler);
			Button->OnClicked.Add(Delegate);
			if (!InitialButton)
			{
				InitialButton = Button;
			}
			return Button;
		};

		auto* Glyph = WidgetTree->ConstructWidget<UCoreKitActionGlyphWidget>();
		Glyph->SetDesiredSizeOverride(FVector2D(42, 42));
		Glyph->SetGlyphSource(Game->GetDoorInput(), Game->GetGlyphs());
		Column->AddChildToVerticalBox(Glyph)->SetHorizontalAlignment(HAlign_Left);
		AddButton(INVTEXT("Toggle door"), GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, ToggleDoor));
		AddButton(INVTEXT("Save checkpoint"), GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, SaveCheckpoint));
		auto* Hold = AddButton(INVTEXT("Hold to block"), GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, ReleaseBlock));
		FScriptDelegate Press;
		Press.BindUFunction(Game, GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, PressBlock));
		Hold->OnPressed.Add(Press);
		FScriptDelegate Release;
		Release.BindUFunction(Game, GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, ReleaseBlock));
		Hold->OnReleased.Add(Release);
		AddButton(INVTEXT("Close IASA for 2 seconds"), GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, SimulateBusy));
		AddButton(INVTEXT("Main menu"), GET_FUNCTION_NAME_CHECKED(UCoreKitExampleGame, BackToMenu));

		Status = MakeText(*WidgetTree, FText::GetEmpty(), 13);
		Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(0, 12, 0, 0));
	}
	return Super::RebuildWidget();
}

void UCoreKitExamplePanel::SetStatus(const FText& Message)
{
	if (Status)
	{
		Status->SetText(Message);
	}
}

void UCoreKitExampleGame::RegisterInput()
{
	if (auto* Player = GetFirstGamePlayer(); Player && InputContext)
	{
		auto* Input = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if (auto* Settings = Input->GetUserSettings())
		{
			Settings->RegisterInputMappingContext(InputContext);
		}
		if (!Input->HasMappingContext(InputContext))
		{
			FModifyContextOptions Options;
			Options.bForceImmediately = true;
			Input->AddMappingContext(InputContext, 0, Options);
		}
		auto* Navigation = Player->GetSubsystem<UCoreKitNavigationSubsystem>();
		Navigation->SetDeviceTrackingEnabled(true);
	}
}

void UCoreKitExampleGame::OnStart()
{
	Super::OnStart();
	if (auto* Frontend = GetSubsystem<UCoreKitFrontendSubsystem>())
	{
		Frontend->OnGameArrived.AddUniqueDynamic(this, &ThisClass::GameArrived);
		Frontend->OnChanged.AddUniqueDynamic(this, &ThisClass::FrontendChanged);
		Frontend->OnCheckpointSaved.AddUniqueDynamic(this, &ThisClass::CheckpointSaved);
	}
	RegisterInput();
	LoadInteractionCatalog();
	if (IsPlayground())
	{
		ShowPlayground();
	}
}

void UCoreKitExampleGame::GameArrived(bool bContinuing)
{
	RegisterInput();
	const FText Message = GetSubsystem<UCoreKitFrontendSubsystem>()->GetStatus();
	ShowPlayground(bContinuing && Message.IsEmpty() ? INVTEXT("Checkpoint restored.") : Message);
}

void UCoreKitExampleGame::FrontendChanged()
{
	if (GetSubsystem<UCoreKitFrontendSubsystem>()->GetState() == ECoreKitFrontendState::Traveling)
	{
		ClosePage();
	}
}

void UCoreKitExampleGame::CheckpointSaved(bool bSucceeded)
{
	ShowStatus(bSucceeded ? INVTEXT("Checkpoint saved.") : INVTEXT("Checkpoint write failed."));
}

TSubclassOf<UCoreKitExampleBlockAction> UCoreKitExampleGame::GetBlockActionClass() const
{
	return BlockActionClass ? BlockActionClass.Get() : UCoreKitExampleBlockAction::StaticClass();
}

void UCoreKitExampleGame::LoadInteractionCatalog()
{
	const auto References = UCoreKitDataAssetLibrary::DiscoverDataContainers(UCoreKitDataContainer::StaticClass(), TEXT("/Game/CoreKitExample/Data"));
	auto* Request = UCoreKitDataLoadRequest::LoadDataContainersAsync(this, References);
	Request->CompletedNative.AddWeakLambda(this, [this](const auto& Loaded, const auto&)
	{
		for (const UCoreKitDataContainer* Container : Loaded)
		{
			const auto* Interaction = Cast<UCoreKitExampleInteraction>(Container->FindDataFragment(UCoreKitExampleInteraction::StaticClass()));
			if (Interaction)
			{
				DoorDuration = FMath::Max(0.0f, Interaction->Duration);
				break;
			}
		}
	});
	Request->Activate();
}

bool UCoreKitExampleGame::IsPlayground() const
{
	return GetWorld() && UWorld::RemovePIEPrefix(GetWorld()->GetOutermost()->GetName()) == PlaygroundMap.ToSoftObjectPath().GetLongPackageName();
}

void UCoreKitExampleGame::ClosePage()
{
	if (auto* Player = GetFirstGamePlayer())
	{
		auto* Navigation = Player->GetSubsystem<UCoreKitNavigationSubsystem>();
		Navigation->ReleaseMenuFocusScope(FocusScope);
	}
	FocusScope.Invalidate();
	if (Panel)
	{
		Panel->RemoveFromParent();
		Panel = nullptr;
	}
}

void UCoreKitExampleGame::ShowPlayground(const FText& Message)
{
	ClosePage();
	auto* Player = GetFirstGamePlayer();
	auto* Controller = Player ? Player->GetPlayerController(GetWorld()) : nullptr;
	if (!Controller)
	{
		return;
	}
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const ACoreKitExampleDoor* Door = FindDoor(GetWorld());
	const FVector DoorLocation = Door ? Door->GetActorLocation() : FVector::ZeroVector;
	const FVector CameraPosition = DoorLocation + FVector(-530, -380, 265);
	auto* Camera = GetWorld()->SpawnActor<ACameraActor>(CameraPosition,
		(DoorLocation + FVector(0, 45, 105) - CameraPosition).Rotation(), Spawn);
	if (Camera)
	{
		Controller->SetViewTarget(Camera);
	}

	Panel = CreateWidget<UCoreKitExamplePanel>(Controller, PanelClass ? PanelClass.Get() : UCoreKitExamplePanel::StaticClass());
	if (!Panel)
	{
		return;
	}
	Panel->SetRenderOpacity(0.f);
	Panel->AddToPlayerScreen();
	Panel->SetStatus(Message);
	Controller->SetShowMouseCursor(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	Controller->SetInputMode(InputMode);
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakPanel = TWeakObjectPtr(Panel.Get())]
	{
		if (Panel && Panel.Get() == WeakPanel.Get())
		{
			FocusScope = GetFirstGamePlayer()->GetSubsystem<UCoreKitNavigationSubsystem>()->PushMenuFocusScope(Panel->GetInitialButton(), Panel);
		}
	}));

	FCoreKitTweenOptions Options;
	Options.Clock = ECoreKitTweenClock::RealTime;
	GetWorld()->GetSubsystem<UCoreKitTweenSubsystem>()->StartSequence(Panel, {{.22f, ECoreKitTweenEase::SmoothStep}}, Options,
		FCoreKitTweenStepUpdated::CreateWeakLambda(Panel, [WeakPanel = TWeakObjectPtr(Panel.Get())](int32, float Alpha)
		{
			if (auto* Widget = WeakPanel.Get()) { Widget->SetRenderOpacity(Alpha); }
		}));
}

void UCoreKitExampleGame::ShowStatus(const FText& Message)
{
	if (Panel)
	{
		Panel->SetStatus(Message);
	}
}

void UCoreKitExampleGame::BackToMenu()
{
	if (auto* Frontend = GetSubsystem<UCoreKitFrontendSubsystem>())
	{
		Frontend->ReturnToMainMenu();
	}
}

void UCoreKitExampleGame::ToggleDoor()
{
	if (auto* Door = IsPlayground() ? FindDoor(GetWorld()) : nullptr)
	{
		Door->SetOpen(!Door->IsOpen());
		ShowStatus(Door->IsOpen() ? INVTEXT("Door open.") : INVTEXT("Door closed."));
	}
}

void UCoreKitExampleGame::SaveCheckpoint()
{
	auto* Frontend = GetSubsystem<UCoreKitFrontendSubsystem>();
	if (!Frontend || !Frontend->SaveCheckpoint())
	{
		ShowStatus(INVTEXT("A checkpoint could not be started."));
	}
}

void UCoreKitExampleGame::PressBlock()
{
	if (auto* Controller = Cast<ACoreKitExampleController>(GetFirstLocalPlayerController()))
	{
		Controller->SetBlockHeld(true);
		ShowStatus(GetBlockStatus());
	}
}

void UCoreKitExampleGame::ReleaseBlock()
{
	if (auto* Controller = Cast<ACoreKitExampleController>(GetFirstLocalPlayerController()))
	{
		Controller->SetBlockHeld(false);
		ShowStatus(GetBlockStatus());
	}
}

FText UCoreKitExampleGame::GetBlockStatus() const
{
	const auto* Controller = Cast<ACoreKitExampleController>(GetFirstLocalPlayerController());
	return Controller && Controller->IsBlocking() ? INVTEXT("Block active.") : INVTEXT("Block inactive.");
}

void UCoreKitExampleGame::ShowBlockStatus()
{
	if (IsPlayground())
	{
		ShowStatus(GetBlockStatus());
	}
}

void UCoreKitExampleGame::SimulateBusy()
{
	if (auto* Controller = Cast<ACoreKitExampleController>(GetFirstLocalPlayerController()))
	{
		Controller->SimulateBusy();
		ShowStatus(INVTEXT("Recovering. Hold Block to start when IASA reopens."));
	}
}
