#include "DPawn.h"

#include "DMoverComponent.h"
#include "DPlayerTuning.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void ADPawn::PlayFoley(USoundBase* Sound) const
{
    if (Sound && IsLocallyControlled() && GetNetMode() != NM_DedicatedServer)
    {
        UGameplayStatics::SpawnSoundAttached(Sound, GetRootComponent(), NAME_None, FVector::ZeroVector,
            EAttachLocation::KeepRelativeOffset, true);
    }
}

void ADPawn::UpdateBootAudio()
{
    const double Now = GetWorld()->GetTimeSeconds();
    const double Elapsed = FMath::Clamp(Now - LastBootAudioTime, 0., .15);
    LastBootAudioTime = Now;
    const bool bAttached = Mover->IsMagneticallyAttached();
    const UDPlayerTuning& Tuning = GetTuning();
    if (bAttached != bBootsWereAttached)
    {
        bBootsWereAttached = bAttached;
        DistanceSinceFootstep = 0.;
        if (bAttached) { PlayFoley(Tuning.BootLatchSound); }
        return;
    }
    if (!bAttached || MoveIntent.IsNearlyZero() || Tuning.BootSteps.IsEmpty())
    {
        DistanceSinceFootstep = 0.;
        return;
    }
    // Relative speed excludes the motion of a platform carrying a stationary pawn.
    DistanceSinceFootstep += Mover->GetSurfaceTravelSpeed() * Elapsed;
    const double StepLength = bRunHeld ? 108. : 82.;
    if (DistanceSinceFootstep >= StepLength)
    {
        DistanceSinceFootstep = FMath::Fmod(DistanceSinceFootstep, StepLength);
        NextBootStep %= Tuning.BootSteps.Num();
        PlayFoley(Tuning.BootSteps[NextBootStep++]);
    }
}
