#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

UCoreKitBasicUIFloatingTextWidget::UCoreKitBasicUIFloatingTextWidget(const FObjectInitializer& I):Super(I) { SetIsFocusable(false); }
TSharedRef<SWidget> UCoreKitBasicUIFloatingTextWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree"));
    if (!WidgetTree->RootWidget)
    {
        auto* Box=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("FloatingText"));
        Icon=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Icon"));
        Text=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("Value"));
        Box->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
        Box->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
        WidgetTree->RootWidget=Box;
    }
    SetVisibility(ESlateVisibility::HitTestInvisible);SetRenderTransformPivot(FVector2D(.5));
    return Super::RebuildWidget();
}
void UCoreKitBasicUIFloatingTextWidget::Start(const FText& Value,const FCoreKitBasicUIFloatingTextStyle& Style,FRandomStream& Random)
{
    TakeWidget();CurrentStyle=Style;
    const float A=Style.HorizontalOffsetRange.HasLowerBound()?Style.HorizontalOffsetRange.GetLowerBoundValue():0.f;
    const float B=Style.HorizontalOffsetRange.HasUpperBound()?Style.HorizontalOffsetRange.GetUpperBoundValue():0.f;
    OffsetX=Random.FRandRange(FMath::Min(A,B),FMath::Max(A,B));Mirror=Style.bRandomMirror && Random.RandRange(0,1)?-1.f:1.f;
    Text->SetText(Value);
    if (Style.Layout!=ECoreKitBasicUIFloatingTextLayout::Icon) { Text->SetFont(Style.Font);Text->SetColorAndOpacity(Style.TextColor); }
    Icon->SetBrush(Style.IconBrush);
    Text->SetVisibility(Style.Layout==ECoreKitBasicUIFloatingTextLayout::Icon?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    Icon->SetVisibility(Style.Layout==ECoreKitBasicUIFloatingTextLayout::Text?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    Animate(0,1,1);
}
void UCoreKitBasicUIFloatingTextWidget::Animate(float Time,float OpacityRate,float DistanceScale)
{
    const float Scale=CurrentStyle.ScaleCurve?CurrentStyle.ScaleCurve->GetFloatValue(Time):1.f;
    const float Opacity=CurrentStyle.OpacityCurve?CurrentStyle.OpacityCurve->GetFloatValue(Time):1.f;
    const FVector Position=CurrentStyle.TranslationCurve?CurrentStyle.TranslationCurve->GetVectorValue(Time):FVector::ZeroVector;
    SetRenderScale(FVector2D(FMath::Clamp(Scale*DistanceScale,0.f,20.f)));
    SetRenderOpacity(FMath::Clamp(Opacity*OpacityRate,0.f,1.f));
    // Preserve the source's screen-space Y inversion and per-spawn X mirroring/offset.
    SetRenderTranslation(FVector2D((Position.X+OffsetX)*Mirror,-Position.Y)*DistanceScale);
}
FText UCoreKitBasicUIFloatingTextWidget::GetValue() const { return Text?Text->GetText():FText::GetEmpty(); }
UCoreKitBasicUIFloatingTextLayerWidget::UCoreKitBasicUIFloatingTextLayerWidget(const FObjectInitializer& I):Super(I) { SetIsFocusable(false); }
TSharedRef<SWidget> UCoreKitBasicUIFloatingTextLayerWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree"));
    if (!WidgetTree->RootWidget) { Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("TextCanvas"));WidgetTree->RootWidget=Canvas; }
    SetVisibility(ESlateVisibility::HitTestInvisible);return Super::RebuildWidget();
}
void UCoreKitBasicUIFloatingTextLayerWidget::AddText(UCoreKitBasicUIFloatingTextWidget* Item)
{
    TakeWidget();auto* CanvasSlot=Canvas->AddChildToCanvas(Item);CanvasSlot->SetAutoSize(true);CanvasSlot->SetAlignment(FVector2D(.5));CanvasSlot->SetPosition(FVector2D::ZeroVector);
}
