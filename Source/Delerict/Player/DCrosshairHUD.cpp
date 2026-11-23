#include "DCrosshairHUD.h"

#include "DPawn.h"
#include "Engine/Canvas.h"

void ADCrosshairHUD::DrawHUD()
{
    Super::DrawHUD();

    const auto* Pawn = Cast<ADPawn>(GetOwningPawn());
    FVector2D Position;
    if (!Canvas || !Pawn || !Pawn->GetCrosshairPosition(Position))
    {
        return;
    }

    const double Length = 9.;
    const double Gap = 3.;
    Position.X = FMath::Clamp(Position.X, Length, FMath::Max(Length, Canvas->ClipX - Length));
    Position.Y = FMath::Clamp(Position.Y, Length, FMath::Max(Length, Canvas->ClipY - Length));

    for (const FVector2D Axis : { FVector2D(1., 0.), FVector2D(0., 1.) })
    {
        for (const double Sign : { -1., 1. })
        {
            const FVector2D Start = Position + Gap * Sign * Axis;
            const FVector2D End = Position + Length * Sign * Axis;
            DrawLine(Start.X, Start.Y, End.X, End.Y, FLinearColor::Black, 3.f);
            DrawLine(Start.X, Start.Y, End.X, End.Y, FLinearColor::White, 1.5f);
        }
    }
}
