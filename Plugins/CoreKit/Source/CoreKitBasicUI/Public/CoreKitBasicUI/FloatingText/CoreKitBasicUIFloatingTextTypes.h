#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "CoreKitBasicUIFloatingTextTypes.generated.h"
class UCurveFloat;
class UCurveVector;

UENUM(BlueprintType)
enum class ECoreKitBasicUIFloatingTextLayout:uint8 { TextAndIcon, Text, Icon };

/** Native equivalent of S_Options. StyleId preserves the fifteen source style identities. */
USTRUCT(BlueprintType)
struct COREKITBASICUI_API FCoreKitBasicUIFloatingTextStyle:public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName StyleId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) ECoreKitBasicUIFloatingTextLayout Layout=ECoreKitBasicUIFloatingTextLayout::Text;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FSlateBrush IconBrush;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UCurveFloat> OpacityCurve;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UCurveFloat> ScaleCurve;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UCurveVector> TranslationCurve;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRandomMirror=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FFloatRange HorizontalOffsetRange;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FSlateColor TextColor=FSlateColor(FLinearColor::White);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FSlateFontInfo Font;
    float GetLifetime() const;
    bool IsUsable() const;
};

UCLASS(BlueprintType)
class COREKITBASICUI_API UCoreKitBasicUIFloatingTextDefinition:public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UDataTable> Styles;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName NormalStyle=TEXT("SmashNormal");
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName HeadStyle=TEXT("SmashCritical");
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName KillStyle=TEXT("SmashKill");
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName ElectricStyle=TEXT("SHOCK");
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="1",ClampMax="256")) int32 InitialPoolSize=5;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="1",ClampMax="256")) int32 MaxTexts=128;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MinDistance=200.f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MaxDistance=2000.f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float ScaleAtMinDistance=2.f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float ScaleAtMaxDistance=.4f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0",ClampMax="1")) float OpacityDecreasing=.3f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D ScreenOffset=FVector2D(50,0);
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bFollowTarget=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0",ClampMax="3")) int32 FractionalDigits=1;
    const FCoreKitBasicUIFloatingTextStyle* FindStyle(FName Id) const;
    bool IsUsable() const;
    float DistanceScale(float Distance) const;
};
