#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextTypes.h"
#include "CoreKitBasicUIFloatingTextWidget.generated.h"
class UTextBlock;
class UImage;
class UCanvasPanel;

/** Entire widget tree and animation are native; no Widget Blueprint is required. */
UCLASS(NotBlueprintable)
class COREKITBASICUI_API UCoreKitBasicUIFloatingTextWidget:public UUserWidget
{
    GENERATED_BODY()
public:
    UCoreKitBasicUIFloatingTextWidget(const FObjectInitializer& Initializer);
    void Start(const FText& Value,const FCoreKitBasicUIFloatingTextStyle& Style,FRandomStream& Random);
    void Animate(float Time,float OpacityRate,float DistanceScale);
    FName GetStyleId() const { return CurrentStyle.StyleId; }
    FText GetValue() const;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY() TObjectPtr<UTextBlock> Text;
    UPROPERTY() TObjectPtr<UImage> Icon;
    UPROPERTY() FCoreKitBasicUIFloatingTextStyle CurrentStyle;
    float OffsetX=0.f,Mirror=1.f;
};

UCLASS(NotBlueprintable)
class COREKITBASICUI_API UCoreKitBasicUIFloatingTextLayerWidget:public UUserWidget
{
    GENERATED_BODY()
public:
    UCoreKitBasicUIFloatingTextLayerWidget(const FObjectInitializer& Initializer);
    void AddText(UCoreKitBasicUIFloatingTextWidget* Text);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY() TObjectPtr<UCanvasPanel> Canvas;
};
