#if !UE_BUILD_SHIPPING

#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Containers/Ticker.h"
#include "CoreKitFrontend/CoreKitFrontendSettings.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "CoreKitLoading/CoreKitLoadingSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Sound/SoundWave.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"
#include "../DMenu.h"
#include "../../LevelDesign/DRoomProgress.h"
#include "../../Player/DPawn.h"

namespace
{
UUserWidget* FindScreen(UWorld* World, bool bOptions)
{
    for (TObjectIterator<UCoreKitFrontendMenu> It; It; ++It)
    {
        if (It->GetWorld() == World && (bOptions
            ? It->GetParent() && It->GetWidgetFromName(TEXT("SelectVideo"))
            : It->IsA<UDMenu>() && It->IsInViewport()))
        {
            return *It;
        }
    }
    return nullptr;
}

bool Click(UUserWidget* Screen, FName Name)
{
    auto* Button = Screen ? Cast<UButton>(Screen->GetWidgetFromName(Name)) : nullptr;
    if (!Button || !Button->GetIsEnabled()) { return false; }
    Button->OnClicked.Broadcast();
    return true;
}

int32 PlayingTitleTracks()
{
    int32 Count = 0;
    for (TObjectIterator<UAudioComponent> It; It; ++It)
    {
        if (const auto* Wave = Cast<USoundWave>(It->Sound); It->IsPlaying() && Wave &&
            Wave->GetFName() == TEXT("Delerict_Ambient_Loop_MainMenu") && Wave->bLooping)
        {
            ++Count;
        }
    }
    return Count;
}

void Capture(const TCHAR* Filename)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / Filename, true, false);
}

bool Finish(bool bPassed, int32 Step)
{
    UE_CLOG(bPassed, LogTemp, Display, TEXT("DELERICT_FRONTEND_SMOKE_PASSED"));
    UE_CLOG(!bPassed, LogTemp, Error, TEXT("DELERICT_FRONTEND_SMOKE_FAIL at step %d"), Step);
    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
    return false;
}

void RunSmoke(UWorld* World)
{
    UGameInstance* Game = World ? World->GetGameInstance() : nullptr;
    if (!Game || !Game->GetSubsystem<UCoreKitFrontendSubsystem>()) { Finish(false, 0); return; }
    const FName EntryMap(*FPackageName::GetShortName(GetDefault<UCoreKitFrontendSettings>()->FirstGameMap.GetLongPackageName()));
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(Game,
        [Game, EntryMap, bSawLoading = false, Step = 0, OriginalMusic = 1.f, ChangedMusic = 0.f, OriginalMode = 0,
            Deadline = FPlatformTime::Seconds() + 55.0](float) mutable
        {
            if (FPlatformTime::Seconds() >= Deadline) { return Finish(false, Step); }
            UWorld* Current = Game->GetWorld();
            if (!Current) { return true; }
            auto* Frontend = Game->GetSubsystem<UCoreKitFrontendSubsystem>();
            const bool bCheckContinue = FParse::Param(FCommandLine::Get(), TEXT("DelerictTestCheckpoint"));
            APlayerController* Player = Current->GetFirstPlayerController();
            UUserWidget* Menu = FindScreen(Current, false);
            UUserWidget* Options = FindScreen(Current, true);
            switch (Step)
            {
            case 0:
                if (!Menu || PlayingTitleTracks() != 1) { break; }
                if (bCheckContinue && Frontend->CanContinue()) { return Finish(false, Step); }
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("MainMenu.png"), true, false);
                ++Step;
                break;
            case 1:
                Step += Click(Menu, TEXT("Settings"));
                break;
            case 2:
                if (!Options) { break; }
                if (PlayingTitleTracks() != 1) { return Finish(false, Step); }
                Capture(TEXT("Settings-Video.png"));
                ++Step;
                break;
            case 3:
                Step += Click(Options, TEXT("SelectAudio"));
                break;
            case 4:
            {
                auto* Slider = Cast<USlider>(Options->GetWidgetFromName(TEXT("MusicVolume")));
                if (!Slider) { return Finish(false, Step); }
                OriginalMusic = Slider->GetValue();
                ChangedMusic = OriginalMusic > .5f ? .37f : .73f;
                Slider->SetValue(ChangedMusic);
                if (!FMath::IsNearlyEqual(Slider->GetValue(), ChangedMusic)) { return Finish(false, Step); }
                Capture(TEXT("Settings-Audio.png"));
                ++Step;
                break;
            }
            case 5:
                Step += Click(Options, TEXT("Back"));
                break;
            case 6:
                Capture(TEXT("Settings-Unsaved.png"));
                ++Step;
                break;
            case 7:
                if (!Click(Options, TEXT("CancelSettingsDialog"))) { return Finish(false, Step); }
                ++Step;
                break;
            case 8:
                if (!Click(Options, TEXT("Apply"))) { return Finish(false, Step); }
                ++Step;
                break;
            case 9:
                if (!Click(Options, TEXT("Back"))) { return Finish(false, Step); }
                ++Step;
                break;
            case 10:
                if (Options) { UE_LOG(LogTemp, Error, TEXT("Back left the applied settings mounted.")); return Finish(false, Step); }
                Step += Click(Menu, TEXT("Settings"));
                break;
            case 11:
            {
                if (!Options) { break; }
                Click(Options, TEXT("SelectAudio"));
                auto* Slider = Cast<USlider>(Options->GetWidgetFromName(TEXT("MusicVolume")));
                if (!Slider || !FMath::IsNearlyEqual(Slider->GetValue(), ChangedMusic))
                {
                    UE_LOG(LogTemp, Error, TEXT("Reopened Music Volume %f, expected %f."), Slider ? Slider->GetValue() : -1.f, ChangedMusic);
                    return Finish(false, Step);
                }
                Slider->SetValue(OriginalMusic);
                if (!Click(Options, TEXT("Apply"))) { UE_LOG(LogTemp, Error, TEXT("Restoring Music Volume did not enable Apply.")); return Finish(false, Step); }
                Click(Options, TEXT("SelectVideo"));
                auto* Sync = Cast<UComboBoxString>(Options->GetWidgetFromName(TEXT("VSync")));
                if (!Sync) { return Finish(false, Step); }
                const int32 OriginalVSync = Sync->GetSelectedIndex();
                Sync->SetSelectedIndex(1 - OriginalVSync);
                if (!Click(Options, TEXT("Discard")) || Sync->GetSelectedIndex() != OriginalVSync) { UE_LOG(LogTemp, Error, TEXT("VSync discard failed.")); return Finish(false, Step); }
                ++Step;
                break;
            }
            case 12:
            {
                auto* Mode = Cast<UComboBoxString>(Options->GetWidgetFromName(TEXT("WindowMode")));
                if (!Mode || !Mode->GetIsEnabled()) { return Finish(false, Step); }
                OriginalMode = Mode->GetSelectedIndex();
                Mode->SetSelectedIndex(OriginalMode == 2 ? 1 : 2);
                if (!Click(Options, TEXT("Apply"))) { return Finish(false, Step); }
                ++Step;
                break;
            }
            case 13:
                Capture(TEXT("Settings-Display.png"));
                ++Step;
                break;
            case 14:
            {
                auto* Mode = CastChecked<UComboBoxString>(Options->GetWidgetFromName(TEXT("WindowMode")));
                if (!Click(Options, TEXT("CancelSettingsDialog")) || Mode->GetSelectedIndex() != OriginalMode)
                {
                    return Finish(false, Step);
                }
                const auto ApplyMode = [&](int32 Index)
                {
                    Mode->SetSelectedIndex(Index);
                    return Click(Options, TEXT("Apply")) && Click(Options, TEXT("AcceptSettingsDialog"))
                        && Mode->GetSelectedIndex() == Index;
                };
                if (!ApplyMode(OriginalMode == 2 ? 1 : 2) || !ApplyMode(OriginalMode)) { return Finish(false, Step); }
                ++Step;
                break;
            }
            case 15:
                Step += Click(Options, TEXT("SelectControls"));
                break;
            case 16:
                Capture(TEXT("Settings-Controls.png"));
                ++Step;
                break;
            case 17:
                Step += Click(Options, TEXT("Back"));
                break;
            case 18:
                if (!Options && Menu) { ++Step; }
                break;
            case 19:
                if (Click(Menu, TEXT("NewGame")))
                {
                    bSawLoading = Game->GetSubsystem<UCoreKitLoadingSubsystem>()->IsLoadingScreenVisible();
                    if (bSawLoading) { Capture(TEXT("NewGameLoading.png")); }
                    ++Step;
                }
                break;
            case 20:
                bSawLoading |= Game->GetSubsystem<UCoreKitLoadingSubsystem>()->IsLoadingScreenVisible();
                if (!Player || !Cast<ADPawn>(Player->GetPawn()) || Frontend->GetState() != ECoreKitFrontendState::Gameplay) { break; }
                if (Current->GetFName() != EntryMap || Player->GetPawn()->GetClass()->GetFName() != TEXT("BP_DPlayer_C")
                    || !bSawLoading || PlayingTitleTracks() != 0)
                {
                    UE_LOG(LogTemp, Error, TEXT("Expected entry %s, BP_DPlayer_C and visible loading. Got %s, %s, loading=%d."),
                        *EntryMap.ToString(), *Current->GetName(), *Player->GetPawn()->GetClass()->GetName(), bSawLoading);
                    return Finish(false, Step);
                }
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Pressed, 1.f));
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Escape, IE_Released, 0.f));
                ++Step;
                break;
            case 21:
                if (!UGameplayStatics::IsGamePaused(Current) || !Menu) { break; }
                if (!Player || Player->CurrentMouseCursor != EMouseCursor::Default) { return Finish(false, Step); }
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PauseMenu.png"), true, false);
                ++Step;
                break;
            case 22:
                Step += Click(Menu, TEXT("Settings"));
                break;
            case 23:
                if (!Options) { break; }
                if (!Click(Options, TEXT("SelectControls")) || !Click(Options, TEXT("Back"))) { return Finish(false, Step); }
                ++Step;
                break;
            case 24:
                Step += Click(Menu, TEXT("Resume"));
                break;
            case 25:
                if (UGameplayStatics::IsGamePaused(Current) || !Player) { break; }
                if (Player->CurrentMouseCursor != EMouseCursor::None) { return Finish(false, Step); }
                if (bCheckContinue && !Frontend->SaveCheckpoint(TEXT("menu-smoke"))) { return Finish(false, Step); }
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_Special_Right, IE_Pressed, 1.f));
                Player->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_Special_Right, IE_Released, 0.f));
                ++Step;
                break;
            case 26:
                if (Click(Menu, TEXT("ReturnToMainMenu")) && Click(Menu, TEXT("Confirm"))) { ++Step; }
                break;
            case 27:
                if (Current->GetFName() == TEXT("L_MainMenu") && Menu && PlayingTitleTracks() == 1)
                {
                    if (!bCheckContinue) { return Finish(true, Step); }
                    if (Frontend->CanContinue() && Click(Menu, TEXT("Continue"))) { ++Step; }
                }
                break;
            case 28:
                if (Current->GetFName() == EntryMap && Frontend->GetState() == ECoreKitFrontendState::Gameplay
                    && Player && Cast<ADPawn>(Player->GetPawn()) && PlayingTitleTracks() == 0)
                {
                    return Finish(true, Step);
                }
                break;
            }
            return true;
        }), .45f);
}

FAutoConsoleCommandWithWorld SmokeCommand(TEXT("Delerict.MenuSmoke"),
    TEXT("Test the Delerict frontend in a standalone development process."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&RunSmoke));

// Run in a fresh process with the isolated profile populated by LevelDesign.PlayableRoute.
void ResumeLabCheckpoint(UWorld* World)
{
    enum class EStep { Continue, LoadedLab, TestLane, ReturnedToLab, ReloadedLatest, ReadLatest, LatestOnDisk, Title, NewGame };
    auto* Game = World->GetGameInstance();
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(Game,
        [Game, Step = EStep::Continue, Deadline = FPlatformTime::Seconds() + 75.](float) mutable
        {
            auto Fail = [&] { return Finish(false, 100 + static_cast<int32>(Step)); };
            if (FPlatformTime::Seconds() > Deadline) { return Fail(); }
            UWorld* Current = Game->GetWorld();
            auto* Player = Current ? Current->GetFirstPlayerController() : nullptr;
            if (!Player) { return true; }
            auto* Frontend = Game->GetSubsystem<UCoreKitFrontendSubsystem>();
            UUserWidget* Menu = FindScreen(Current, false);
            const bool bPlaying = Frontend->GetState() == ECoreKitFrontendState::Gameplay && Player->GetPawn();
            switch (Step)
            {
            case EStep::Continue:
                if (Frontend->CanContinue() && Click(Menu, TEXT("Continue"))) { Step = EStep::LoadedLab; }
                break;
            case EStep::LoadedLab:
            case EStep::ReturnedToLab:
            case EStep::ReloadedLatest:
            case EStep::LatestOnDisk:
                if (bPlaying && Current->GetFName() == TEXT("L_DLevelDesignLab"))
                {
                    auto* Progress = Current->GetSubsystem<UDRoomProgress>();
                    const auto Checkpoint = Progress->GetCheckpoint(*Player);
                    const FVector Latest(1975., 0., 140.);
                    const FVector Expected = (Step == EStep::ReloadedLatest || Step == EStep::LatestOnDisk) ?
                        Latest : FVector(1450., 0., 100.);
                    if (!Checkpoint || !Checkpoint->GetLocation().Equals(Expected, 1.) ||
                        !Player->GetPawn()->GetActorLocation().Equals(Expected, 10.) ||
                        UGameplayStatics::IsGamePaused(Current) || Menu)
                    {
                        UE_LOG(LogTemp, Error, TEXT("Expected %s. Checkpoint %s, pawn %s."), *Expected.ToString(),
                            Checkpoint ? *Checkpoint->GetLocation().ToString() : TEXT("missing"),
                            *Player->GetPawn()->GetActorLocation().ToString());
                        return Fail();
                    }
                    if (Step == EStep::LoadedLab)
                    {
                        UGameplayStatics::OpenLevel(Current, TEXT("/Game/Maps/L_DTestLane"));
                        Step = EStep::TestLane;
                    }
                    else if (Step == EStep::ReturnedToLab)
                    {
                        // Reaching two checkpoints before the first disk write completes must retain the newest one.
                        if (!Progress->SetCheckpoint(*Player, FTransform(FVector(1850., 0., 140.))) ||
                            !Progress->SetCheckpoint(*Player, FTransform(Latest)) || !UDMenu::OpenPause(*Player) ||
                            !Click(FindScreen(Current, false), TEXT("RestartCheckpoint"))) { return Fail(); }
                        Step = EStep::ReloadedLatest;
                    }
                    else
                    {
                        Frontend->ReturnToMainMenu();
                        Step = Step == EStep::ReloadedLatest ? EStep::ReadLatest : EStep::Title;
                    }
                }
                break;
            case EStep::TestLane:
                if (bPlaying && Current->GetFName() == TEXT("L_DTestLane"))
                {
                    if (!UDMenu::OpenPause(*Player) || !UGameplayStatics::IsGamePaused(Current) ||
                        !Click(FindScreen(Current, false), TEXT("RestartCheckpoint"))) { return Fail(); }
                    Step = EStep::ReturnedToLab;
                }
                break;
            case EStep::Title:
                if (Frontend->CanContinue() && Click(Menu, TEXT("NewGame")) && Click(Menu, TEXT("Confirm")))
                {
                    Step = EStep::NewGame;
                }
                break;
            case EStep::ReadLatest:
                if (Frontend->CanContinue() && Click(Menu, TEXT("Continue"))) { Step = EStep::LatestOnDisk; }
                break;
            case EStep::NewGame:
                if (bPlaying)
                {
                    if (!UDMenu::OpenPause(*Player)) { return Fail(); }
                    Menu = FindScreen(Current, false);
                    const auto* Button = Menu ? Cast<UButton>(Menu->GetWidgetFromName(TEXT("RestartCheckpoint"))) : nullptr;
                    if (!Button || Button->GetIsEnabled() || !Click(Menu, TEXT("Resume"))) { return Fail(); }
                    UE_LOG(LogTemp, Display, TEXT("DELERICT_CHECKPOINT_COLD_CONTINUE_PAUSE_NEW_GAME_PASSED"));
                    return Finish(true, 100 + static_cast<int32>(Step));
                }
                break;
            }
            return true;
        }), .15f);
}

FAutoConsoleCommandWithWorld ResumeLabCommand(TEXT("Delerict.ResumeLabCheckpoint"),
    TEXT("Verify cold Continue and the pause checkpoint action with a saved LevelDesign.PlayableRoute profile."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&ResumeLabCheckpoint));
}

#endif
