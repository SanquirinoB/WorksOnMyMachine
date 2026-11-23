#include "DLoadingScreen.h"

#include "CoreKitLoading/CoreKitLoadingSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DLoadingScreen"

namespace
{
constexpr float MinimumDisplaySeconds = 1.25f;

TSharedRef<STextBlock> Caption(const FText& Text, FName Typeface, int32 Size, FColor Color)
{
    return SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle(Typeface, Size))
        .ColorAndOpacity(FLinearColor(Color)).SimpleTextMode(true);
}
}

TSharedRef<SWidget> UDLoadingScreen::MakeScreen()
{
    const ISlateStyle* Style = FSlateStyleRegistry::FindSlateStyle(TEXT("Delerict.Loading"));
    check(Style);
    const auto* White = FCoreStyle::Get().GetBrush("WhiteBrush");
    const FColor Ink(243, 242, 246);
    const FColor Muted(187, 181, 198);
    const FLinearColor Edge(FColor(70, 54, 75));

    return SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(FColor(19, 15, 25)))
        .Padding(0).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
        [
            SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [
                SNew(SBox).WidthOverride(1200).HeightOverride(675).HAlign(HAlign_Center).VAlign(VAlign_Center)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        SNew(SBorder).BorderImage(White).BorderBackgroundColor(Edge).Padding(1)
                        [ SNew(SImage).Image(Style->GetBrush("Team")) ]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(70, 0, 0, 0)
                    [
                        SNew(SBox).WidthOverride(320)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot().AutoHeight().Padding(3, 0, 0, 26)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [ SNew(SCircularThrobber).NumPieces(8).Radius(13.f) ]
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0)
                                [ Caption(LOCTEXT("Loading", "Loading…"), "BoldCondensed", 42, Ink) ]
                            ]
                            + SVerticalBox::Slot().AutoHeight().Padding(3, 0)
                            [ SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(White).BorderBackgroundColor(Edge)] ]
                            + SVerticalBox::Slot().AutoHeight().Padding(3, 27, 0, 0)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                                [ SNew(SImage).Image(Style->GetBrush("Codex")) ]
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 0)
                                [
                                    SNew(SVerticalBox)
                                    + SVerticalBox::Slot().AutoHeight()
                                    [ Caption(LOCTEXT("Tool", "Built with"), "Regular", 13, Muted) ]
                                    + SVerticalBox::Slot().AutoHeight()
                                    [ Caption(LOCTEXT("Codex", "Codex"), "Bold", 23, Ink) ]
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> UDLoadingScreen::RebuildWidget()
{
    return MakeScreen();
}

void UDLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    auto* Loading = Collection.InitializeDependency<UCoreKitLoadingSubsystem>();
    if (FApp::CanEverRender() && !IsRunningDedicatedServer())
    {
        Loading->ConfigureLoadingScreen(UDLoadingScreen::StaticClass(), true);
        FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisClass::PrepareMap);
        FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::HandleGameStart);
    }
}

void UDLoadingScreenSubsystem::PrepareMap(const FWorldContext& Context, const FString&)
{
    if (Context.OwningGameInstance != GetGameInstance() || GetGameInstance()->IsDedicatedServerInstance())
    {
        return;
    }
    bPreparedFirstMap = true;
    // The frontend holds its own readiness request. Enable CoreKit's map tracking
    // as well so initial startup and direct OpenLevel calls have the same screen.
    GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>()->ConfigureLoadingScreen(UDLoadingScreen::StaticClass(), true);
    if (IsMoviePlayerEnabled() && GetMoviePlayer()->IsInitialized())
    {
        FLoadingScreenAttributes Attributes;
        Attributes.WidgetLoadingScreen = UDLoadingScreen::MakeScreen();
        Attributes.MinimumLoadingScreenDisplayTime = MinimumDisplaySeconds;
        GetMoviePlayer()->SetupLoadingScreen(Attributes);
        return;
    }
    ShowViewportScreen();
}

void UDLoadingScreenSubsystem::HandleGameStart(UGameInstance* Game)
{
    // PIE can duplicate its first world without sending a PreLoadMap event.
    if (Game == GetGameInstance() && !bPreparedFirstMap && !Game->IsDedicatedServerInstance())
    {
        ShowViewportScreen();
    }
}

void UDLoadingScreenSubsystem::ShowViewportScreen()
{
    auto* Loading = GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>();
    if (!ViewportRequest.IsValid())
    {
        ViewportRequest = Loading->BeginLoadingRequest(this, LOCTEXT("MapLoad", "Map loading"));
    }
    FTSTicker::RemoveTicker(ReleaseTicker);
    ReleaseTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
    {
        ReleaseTicker.Reset();
        ReleaseViewportScreen();
        return false;
    }), MinimumDisplaySeconds);
}

void UDLoadingScreenSubsystem::ReleaseViewportScreen()
{
    if (ViewportRequest.IsValid())
    {
        GetGameInstance()->GetSubsystem<UCoreKitLoadingSubsystem>()->EndLoading(ViewportRequest);
        ViewportRequest.Invalidate();
    }
}

void UDLoadingScreenSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
    FWorldDelegates::OnStartGameInstance.RemoveAll(this);
    FTSTicker::RemoveTicker(ReleaseTicker);
    ReleaseViewportScreen();
    Super::Deinitialize();
}

#undef LOCTEXT_NAMESPACE
