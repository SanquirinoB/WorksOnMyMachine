#include "CoreKitBasicUI/FloatingText/CoreKitBasicUIFloatingTextTypes.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

float FCoreKitBasicUIFloatingTextStyle::GetLifetime() const
{
    float End=0.f,Min=0.f,Max=0.f;
    if (OpacityCurve) { OpacityCurve->GetTimeRange(Min,Max);End=FMath::Max(End,Max); }
    if (ScaleCurve) { ScaleCurve->GetTimeRange(Min,Max);End=FMath::Max(End,Max); }
    if (TranslationCurve) { TranslationCurve->GetTimeRange(Min,Max);End=FMath::Max(End,Max); }
    return FMath::IsFinite(End)?End:0.f;
}
bool FCoreKitBasicUIFloatingTextStyle::IsUsable() const
{ return !StyleId.IsNone() && OpacityCurve && ScaleCurve && TranslationCurve && GetLifetime()>0.f; }
const FCoreKitBasicUIFloatingTextStyle* UCoreKitBasicUIFloatingTextDefinition::FindStyle(FName Id) const
{
    if (!Styles || Styles->GetRowStruct()!=FCoreKitBasicUIFloatingTextStyle::StaticStruct()) return nullptr;
    for (const auto& Pair:Styles->GetRowMap())
    {
        const auto* Row=reinterpret_cast<const FCoreKitBasicUIFloatingTextStyle*>(Pair.Value);
        if (Row->StyleId==Id) return Row;
    }
    return nullptr;
}
bool UCoreKitBasicUIFloatingTextDefinition::IsUsable() const
{
    return Styles && Styles->GetRowStruct()==FCoreKitBasicUIFloatingTextStyle::StaticStruct() &&
        FMath::IsFinite(MinDistance) && FMath::IsFinite(MaxDistance) && MaxDistance>MinDistance && MinDistance>=0.f &&
        FMath::IsFinite(ScaleAtMinDistance) && FMath::IsFinite(ScaleAtMaxDistance) && ScaleAtMinDistance>=ScaleAtMaxDistance && ScaleAtMaxDistance>0.f &&
        InitialPoolSize>0 && MaxTexts>=InitialPoolSize && MaxTexts<=256 &&
        FMath::IsFinite(OpacityDecreasing) && OpacityDecreasing>=0.f && OpacityDecreasing<=1.f &&
        FindStyle(NormalStyle) && FindStyle(NormalStyle)->IsUsable();
}
float UCoreKitBasicUIFloatingTextDefinition::DistanceScale(float Distance) const
{
    if (!FMath::IsFinite(Distance) || MaxDistance<=MinDistance) return 1.f;
    return FMath::Lerp(ScaleAtMinDistance,ScaleAtMaxDistance,FMath::Clamp((Distance-MinDistance)/(MaxDistance-MinDistance),0.f,1.f));
}
