#if !UE_BUILD_SHIPPING

#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Containers/Ticker.h"
#include "CoreKitFrontend/CoreKitFrontendSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundWave.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"
#include "../DMenu.h"
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
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(Game,
        [Game, Step = 0, OriginalMusic = 1.f, ChangedMusic = 0.f, OriginalMode = 0, Deadline = FPlatformTime::Seconds() + 55.0](float) mutable
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
                Step += Click(Menu, TEXT("NewGame"));
                break;
            case 20:
                if (!Player || !Cast<ADPawn>(Player->GetPawn()) || Frontend->GetState() != ECoreKitFrontendState::Gameplay) { break; }
                if (Current->GetFName() != TEXT("L_DHangar") || PlayingTitleTracks() != 0) { return Finish(false, Step); }
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
                if (Current->GetFName() == TEXT("L_DHangar") && Frontend->GetState() == ECoreKitFrontendState::Gameplay
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
}

#endif
