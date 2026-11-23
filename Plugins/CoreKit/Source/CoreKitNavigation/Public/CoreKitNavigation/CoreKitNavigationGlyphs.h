#pragma once

#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "CoreKitNavigationGlyphs.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class COREKITNAVIGATION_API UCoreKitNavigationGlyphs : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	UTexture2D* FindGlyph(FKey Key) const;

private:
	UPROPERTY(EditAnywhere, Category = "CoreKit|Navigation")
	TMap<FKey, TObjectPtr<UTexture2D>> Glyphs;
};
