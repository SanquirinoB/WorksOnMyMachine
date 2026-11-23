#pragma once

#include "Blueprint/UserWidget.h"
#include "DInventoryHUD.generated.h"

class ADPawn;
class UBorder;
class UTextBlock;
class UWrapBox;

UCLASS()
class UDInventoryHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    void Refresh(const ADPawn& Pawn);

private:
    TSharedRef<SWidget> RebuildWidget() override;
    void BuildSlots(const ADPawn& Pawn);

    struct FItemSlot
    {
        FName Type;
        TWeakObjectPtr<UBorder> Card;
        TWeakObjectPtr<UTextBlock> Quantity;
        int32 Count = INDEX_NONE;
        bool bHasHotkey = false;
        bool bSelected = false;
    };

    UPROPERTY(Transient) TObjectPtr<UWrapBox> Items;
    TArray<FItemSlot> ItemSlots;
    TWeakObjectPtr<const ADPawn> DisplayedPawn;
    double NextRefresh = 0.;
};
