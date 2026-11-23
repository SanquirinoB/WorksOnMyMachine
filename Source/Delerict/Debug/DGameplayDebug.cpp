#include "DGameplayDebug.h"

#if DELERICT_WITH_GAMEPLAY_DEBUG

#include "Algo/Find.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/OutputDevice.h"
#include "UObject/ObjectKey.h"

namespace DGameplayDebug
{
    namespace
    {
        struct FChannelInfo
        {
            EChannel Channel;
            const TCHAR* Name;
            FLinearColor Color;
        };

        constexpr FChannelInfo Channels[] = {
            { EChannel::Tuning, TEXT("Tuning"), FLinearColor(1.f, 1.f, 1.f) },
            { EChannel::Flight, TEXT("Flight"), FLinearColor(0.05f, 0.75f, 1.f) },
            { EChannel::Aim, TEXT("Aim"), FLinearColor(0.05f, 1.f, 0.2f) },
            { EChannel::Collision, TEXT("Collision"), FLinearColor(1.f, 0.05f, 0.05f) },
            { EChannel::Throws, TEXT("Throws"), FLinearColor(1.f, 0.35f, 0.02f) },
            { EChannel::Weapons, TEXT("Weapons"), FLinearColor(1.f, 0.05f, 0.65f) },
            { EChannel::Magnet, TEXT("Magnet"), FLinearColor(0.4f, 0.12f, 1.f) },
            { EChannel::Jump, TEXT("Jump"), FLinearColor(1.f, 0.9f, 0.05f) },
        };

        constexpr EChannel AllChannels = []
        {
            EChannel Mask = EChannel::None;
            for (const FChannelInfo& Info : Channels)
            {
                Mask |= Info.Channel;
            }
            return Mask;
        }();

        constexpr float ShapeLifetime = 0.08f;
        constexpr float LineThickness = 1.5f;
        constexpr TCHAR CommandUsage[] = TEXT("Delerict.Debug On <channel ...|All> | Off [channel ...|All] | List");

        struct FWorldChannels
        {
            FWorldChannels()
            {
                CleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda([this](UWorld* World, bool, bool)
                {
                    Active.Remove(World);
                });
            }

            ~FWorldChannels()
            {
                FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);
            }

            TMap<TObjectKey<UWorld>, EChannel> Active;
            FDelegateHandle CleanupHandle;
        };

        TMap<TObjectKey<UWorld>, EChannel>& GetActiveChannels()
        {
            static FWorldChannels WorldChannels;
            return WorldChannels.Active;
        }

        bool CanUseWorld(UWorld* World)
        {
            return IsInGameThread() && World && World->IsGameWorld();
        }

        FString DescribeChannels(EChannel Mask)
        {
            TArray<const TCHAR*, TInlineAllocator<UE_ARRAY_COUNT(Channels)>> Names;
            for (const FChannelInfo& Info : Channels)
            {
                if (EnumHasAnyFlags(Mask, Info.Channel))
                {
                    Names.Add(Info.Name);
                }
            }
            return Names.IsEmpty() ? FString(TEXT("None")) : FString::Join(Names, TEXT(", "));
        }

        void RunConsoleCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
        {
            if (!CanUseWorld(World))
            {
                Output.Log(TEXT("Delerict.Debug requires an active game world."));
                return;
            }

            auto& ActiveChannels = GetActiveChannels();
            EChannel Active = ActiveChannels.FindRef(World);
            if (Args.Num() == 1 && Args[0].Equals(TEXT("List"), ESearchCase::IgnoreCase))
            {
                Output.Logf(TEXT("Delerict.Debug active: %s"), *DescribeChannels(Active));
                Output.Logf(TEXT("Delerict.Debug available: %s"), *DescribeChannels(AllChannels));
                return;
            }

            const bool bEnable = !Args.IsEmpty() && Args[0].Equals(TEXT("On"), ESearchCase::IgnoreCase);
            const bool bDisable = !Args.IsEmpty() && Args[0].Equals(TEXT("Off"), ESearchCase::IgnoreCase);
            if ((!bEnable && !bDisable) || (bEnable && Args.Num() == 1))
            {
                Output.Log(CommandUsage);
                return;
            }

            EChannel Requested = Args.Num() == 1 ? AllChannels : EChannel::None;
            for (int32 Index = 1; Index < Args.Num(); ++Index)
            {
                const FString& Name = Args[Index];
                if (Name.Equals(TEXT("All"), ESearchCase::IgnoreCase))
                {
                    Requested = AllChannels;
                    continue;
                }

                const FChannelInfo* Info = Algo::FindByPredicate(Channels, [&Name](const FChannelInfo& Entry)
                {
                    return Name.Equals(Entry.Name, ESearchCase::IgnoreCase);
                });
                if (!Info)
                {
                    Output.Logf(TEXT("Delerict.Debug unknown channel '%s'. Use Delerict.Debug List."), *Name);
                    return;
                }
                Requested |= Info->Channel;
            }

            Active = bEnable ? Active | Requested : Active & ~Requested;
            if (Active == EChannel::None)
            {
                ActiveChannels.Remove(World);
            }
            else
            {
                ActiveChannels.Add(World, Active);
            }
            Output.Logf(TEXT("Delerict.Debug active: %s"), *DescribeChannels(Active));
        }

        FAutoConsoleCommandWithWorldArgsAndOutputDevice ConsoleCommand(
            TEXT("Delerict.Debug"), CommandUsage,
            FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RunConsoleCommand));

        const FChannelInfo* GetEnabledChannel(UWorld* World, EChannel Channel)
        {
            return IsEnabled(World, Channel) ? Algo::FindBy(Channels, Channel, &FChannelInfo::Channel) : nullptr;
        }
    }

    bool IsEnabled(UWorld* World, EChannel Channel)
    {
        return CanUseWorld(World) && EnumHasAnyFlags(GetActiveChannels().FindRef(World), Channel);
    }

    void DrawArrow(UWorld* World, EChannel Channel, const FVector& Start, const FVector& End)
    {
        if (const FChannelInfo* Info = GetEnabledChannel(World, Channel))
        {
            UKismetSystemLibrary::DrawDebugArrow(World, Start, End, 10.f, Info->Color, ShapeLifetime, LineThickness);
        }
    }

    void DrawLine(UWorld* World, EChannel Channel, const FVector& Start, const FVector& End)
    {
        if (const FChannelInfo* Info = GetEnabledChannel(World, Channel))
        {
            UKismetSystemLibrary::DrawDebugLine(World, Start, End, Info->Color, ShapeLifetime, LineThickness);
        }
    }

    void DrawCapsule(UWorld* World, EChannel Channel, const FVector& Center, float HalfHeight, float Radius,
        const FRotator& Rotation)
    {
        if (const FChannelInfo* Info = GetEnabledChannel(World, Channel))
        {
            UKismetSystemLibrary::DrawDebugCapsule(World, Center, HalfHeight, Radius, Rotation, Info->Color,
                ShapeLifetime, LineThickness);
        }
    }

    void DrawLabel(UWorld* World, EChannel Channel, const FVector& Location, const FString& Text, float Duration)
    {
        if (const FChannelInfo* Info = GetEnabledChannel(World, Channel))
        {
            const FString Label = FString::Printf(TEXT("[%s] %s"), Info->Name, *Text);
            UKismetSystemLibrary::DrawDebugString(World, Location, Label, nullptr, Info->Color, FMath::Max(0.f, Duration));
        }
    }
}

#endif
