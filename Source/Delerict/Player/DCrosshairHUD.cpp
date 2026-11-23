#include "DCrosshairHUD.h"

#include "DPawn.h"
#include "../Frontend/DInventoryHUD.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void ADCrosshairHUD::PostRender()
{
    Super::PostRender();
    const auto* Pawn = Cast<ADPawn>(GetOwningPawn());
    const bool bVisible = Pawn && bShowHUD && !bShowDebugInfo && !UGameplayStatics::IsGamePaused(this);
    if (bVisible && !Inventory)
    {
        if (auto* Player = GetOwningPlayerController(); Player && Player->GetLocalPlayer())
        {
            auto* Widget = CreateWidget<UDInventoryHUD>(Player);
            if (Widget && Widget->AddToPlayerScreen()) { Inventory = Widget; }
        }
    }
    if (Inventory)
    {
        const auto Visibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
        if (Inventory->GetVisibility() != Visibility) { Inventory->SetVisibility(Visibility); }
        if (bVisible) { Inventory->Refresh(*Pawn); }
    }
}

void ADCrosshairHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Inventory) { Inventory->RemoveFromParent(); }
    Super::EndPlay(Reason);
}

void ADCrosshairHUD::DrawHUD()
{
    Super::DrawHUD();

    const auto* Pawn = Cast<ADPawn>(GetOwningPawn());
    FVector2D Position;
    if (!Canvas || !Pawn || UGameplayStatics::IsGamePaused(this) || !Pawn->GetCrosshairPosition(Position))
    {
        return;
    }

    const double Length = 9.;
    Position.X = FMath::Clamp(Position.X, Length, FMath::Max(Length, Canvas->ClipX - Length));
    Position.Y = FMath::Clamp(Position.Y, Length, FMath::Max(Length, Canvas->ClipY - Length));

    if (Pawn->IsAimingWithGamepad())
    {
        const double Gap = 3.;
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

    if (const float Charge = Pawn->GetThrowCharge(); Charge > 0.f)
    {
        const double Top = FMath::Max(4., Position.Y - 16.);
        const double Center = FMath::Clamp(Position.X, 14., FMath::Max(14., Canvas->ClipX - 14.));
        DrawLine(Center - 14., Top, Center + 14., Top, FLinearColor::Black, 4.f);
        DrawLine(Center - 14., Top, Center - 14. + 28. * Charge, Top, FLinearColor::White, 2.f);
    }
}
