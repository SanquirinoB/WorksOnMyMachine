#pragma once

#if DELERICT_WITH_GAMEPLAY_DEBUG

#include "CoreMinimal.h"

class UWorld;

namespace DGameplayDebug
{
    enum class EChannel : uint8
    {
        None = 0,
        Tuning = 1 << 0,
        Flight = 1 << 1,
        Aim = 1 << 2,
        Collision = 1 << 3,
        Throws = 1 << 4,
        Weapons = 1 << 5,
        Magnet = 1 << 6,
        Jump = 1 << 7,
    };
    ENUM_CLASS_FLAGS(EChannel);

    // Guard diagnostic data collection with the build flag and this check before formatting labels or gathering values.
    bool IsEnabled(UWorld* World, EChannel Channel);

    void DrawArrow(UWorld* World, EChannel Channel, const FVector& Start, const FVector& End);
    void DrawLine(UWorld* World, EChannel Channel, const FVector& Start, const FVector& End);
    void DrawCapsule(UWorld* World, EChannel Channel, const FVector& Center, float HalfHeight, float Radius,
        const FRotator& Rotation);
    void DrawLabel(UWorld* World, EChannel Channel, const FVector& Location, const FString& Text, float Duration = 0.f);
}

#endif
