#include "CoreKitLoading/CoreKitLoadingSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "CoreKitLoading/CoreKitOpenLevelRequest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
class SCoreKitLoadingOverlay final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCoreKitLoadingOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&, const TSharedRef<SWidget>& Content)
	{
		ChildSlot[Content];
	}

	bool SupportsKeyboardFocus() const override { return true; }
	FReply OnKeyDown(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }
	FReply OnKeyUp(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }
	FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent&) override { return FReply::Handled(); }
	FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }
	FReply OnMouseWheel(const FGeometry&, const FPointerEvent&) override { return FReply::Handled(); }

	bool HasPainted() const { return bPainted; }

	int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Clip,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		bPainted = true;
		return SCompoundWidget::OnPaint(Args, Geometry, Clip, Elements, Layer, Style, bParentEnabled);
	}

private:
	mutable bool bPainted = false;
};
}

bool UCoreKitLoadingSubsystem::HasOverlayPainted() const
{
	return Overlay.IsValid() && StaticCastSharedPtr<SCoreKitLoadingOverlay>(Overlay)->HasPainted();
}

void UCoreKitLoadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisClass::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMap);
	if (GEngine)
	{
		GEngine->OnTravelFailure().AddUObject(this, &ThisClass::OnTravelFailure);
	}
}

void UCoreKitLoadingSubsystem::Deinitialize()
{
	bShuttingDown = true;
	FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	if (GEngine)
	{
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	ScreenClass = nullptr;
	bTrackTravel = false;
	bTravelInProgress = false;
	if (ActiveTravel)
	{
		ActiveTravel->Finish(ECoreKitTravelResult::Cancelled);
	}
	ActiveRequests.Reset();
	RefreshPresentation();
	Super::Deinitialize();
}

void UCoreKitLoadingSubsystem::ConfigureLoadingScreen(TSubclassOf<UUserWidget> WidgetClass, bool bTrackMapTravel)
{
	if (ScreenClass != WidgetClass)
	{
		HidePresentation();
		ScreenClass = WidgetClass;
	}
	bTrackTravel = bTrackMapTravel;
	if (!bTrackTravel)
	{
		bTravelInProgress = false;
	}
	RefreshPresentation();
}

FGuid UCoreKitLoadingSubsystem::BeginLoading()
{
	return BeginLoadingRequest(nullptr, FText::GetEmpty());
}

FGuid UCoreKitLoadingSubsystem::BeginLoadingRequest(UObject* Owner, FText Reason)
{
	if (bShuttingDown)
	{
		return {};
	}

	const FGuid Request = FGuid::NewGuid();
	ActiveRequests.Add(Request, { GetPathNameSafe(Owner), MoveTemp(Reason), FPlatformTime::Seconds() });
	RefreshPresentation();
	return Request;
}

TArray<FCoreKitLoadingRequestInfo> UCoreKitLoadingSubsystem::GetActiveLoadingRequests() const
{
	const double Now = FPlatformTime::Seconds();
	TArray<FCoreKitLoadingRequestInfo> Results;
	Results.Reserve(ActiveRequests.Num());
	for (const auto& Entry : ActiveRequests)
	{
		Results.Add({ Entry.Key, Entry.Value.Owner, Entry.Value.Reason, Now - Entry.Value.StartedAt });
	}
	return Results;
}

FGuid UCoreKitLoadingSubsystem::GetActiveTravelId() const
{
	return ActiveTravel ? ActiveTravel->TravelId : FGuid();
}

bool UCoreKitLoadingSubsystem::ResolveTravelCondition(FGuid TravelId, FName Condition, bool bSucceeded)
{
	return ActiveTravel && TravelId.IsValid() && ActiveTravel->TravelId == TravelId
		&& ActiveTravel->ResolveCondition(Condition, bSucceeded);
}

bool UCoreKitLoadingSubsystem::EndLoading(FGuid Request)
{
	if (ActiveRequests.Remove(Request) == 0)
	{
		return false;
	}
	RefreshPresentation();
	return true;
}

void UCoreKitLoadingSubsystem::OnPreLoadMap(const FWorldContext& Context, const FString&)
{
	if (bTrackTravel && Context.OwningGameInstance == GetGameInstance())
	{
		bTravelInProgress = true;
		RefreshPresentation();
	}
}

void UCoreKitLoadingSubsystem::OnPostLoadMap(UWorld* World)
{
	if (World && World->GetGameInstance() == GetGameInstance())
	{
		bTravelInProgress = false;
		if (ActiveTravel)
		{
			ActiveTravel->HandleMapLoaded(World);
		}
		RefreshPresentation();
	}
}

void UCoreKitLoadingSubsystem::OnTravelFailure(UWorld* World, ETravelFailure::Type, const FString&)
{
	if (bTravelInProgress && (!World || World->GetGameInstance() == GetGameInstance()))
	{
		bTravelInProgress = false;
		RefreshPresentation();
	}
	if (ActiveTravel && World && World->GetGameInstance() == GetGameInstance())
	{
		ActiveTravel->Finish(ECoreKitTravelResult::TravelFailed);
	}
}

void UCoreKitLoadingSubsystem::RefreshPresentation()
{
	if (bRefreshing)
	{
		return;
	}
	{
		TGuardValue<bool> Guard(bRefreshing, true);
		UpdatePresentation();
	}
	const bool bVisible = IsLoadingScreenVisible();
	if (bPublishedVisible != bVisible)
	{
		bPublishedVisible = bVisible;
		OnLoadingScreenVisibilityChanged.Broadcast(bVisible);
	}
}

void UCoreKitLoadingSubsystem::UpdatePresentation()
{
	if (!IsLoading() || !ScreenClass)
	{
		HidePresentation();
		return;
	}

	UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient();
	if (!Viewport || !FSlateApplication::IsInitialized() || Overlay.IsValid())
	{
		return;
	}

	const TSubclassOf<UUserWidget> RequestedClass = ScreenClass;
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetGameInstance(), RequestedClass);
	if (!Widget)
	{
		return;
	}

	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	const TSharedRef<SWidget> Panel = SNew(SCoreKitLoadingOverlay, Widget->TakeWidget());
	if (!IsLoading() || ScreenClass != RequestedClass)
	{
		return;
	}

	Screen = Widget;
	AttachedViewport = Viewport;
	Overlay = Panel;
	bPreviousIgnoreInput = Viewport->IgnoreInput();
	Viewport->SetIgnoreInput(true);
	Viewport->AddViewportWidgetContent(Panel, MAX_int32);

	FSlateApplication& Slate = FSlateApplication::Get();
	for (ULocalPlayer* Player : GetGameInstance()->GetLocalPlayers())
	{
		if (const TSharedPtr<FSlateUser> User = Player->GetSlateUser())
		{
			const uint32 Index = User->GetUserIndex();
			PreviousFocus.Add({ Index, Slate.GetUserFocusedWidget(Index) });
			Slate.SetUserFocus(Index, Panel);
		}
	}
}

void UCoreKitLoadingSubsystem::HidePresentation()
{
	if (!Overlay.IsValid())
	{
		return;
	}

	const TSharedRef<SWidget> Panel = Overlay.ToSharedRef();
	Overlay.Reset();
	if (UGameViewportClient* Viewport = AttachedViewport.Get())
	{
		Viewport->RemoveViewportWidgetContent(Panel);
		Viewport->SetIgnoreInput(bPreviousIgnoreInput);
	}
	AttachedViewport.Reset();
	Screen = nullptr;

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication& Slate = FSlateApplication::Get();
		for (const FPreviousFocus& Entry : PreviousFocus)
		{
			const TSharedPtr<SWidget> Current = Slate.GetUserFocusedWidget(Entry.UserIndex);
			if (!Current || Current == Panel)
			{
				const TSharedPtr<SWidget> Previous = Entry.Widget.Pin();
				if (!Previous || !Slate.SetUserFocus(Entry.UserIndex, Previous))
				{
					Slate.SetUserFocusToGameViewport(Entry.UserIndex);
				}
			}
		}
	}
	PreviousFocus.Reset();
}
