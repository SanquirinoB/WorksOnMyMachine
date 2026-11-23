#pragma once

#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "CoreKitInputIconSet.generated.h"

class UTexture2D;

/** How one key is shown to the player: an icon, a text label, or both. */
USTRUCT(BlueprintType)
struct COREKITNAVIGATION_API FCoreKitKeyPresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Shown when there is no icon, or next to it when the consumer chooses to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CoreKit|Navigation")
	FText DisplayName;

	bool IsSet() const { return !Icon.IsNull() || !DisplayName.IsEmpty(); }
};

/** Key presentations for one device family, for example keyboard and mouse, or one gamepad type. */
UCLASS(BlueprintType)
class COREKITNAVIGATION_API UCoreKitInputIconSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	bool FindKey(FKey Key, FCoreKitKeyPresentation& OutPresentation) const;

	/** Authoring helper for editor scripts that build icon sets from glyph packs. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetKey(FKey Key, const FCoreKitKeyPresentation& Presentation);

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	int32 Num() const { return Keys.Num(); }

private:
	UPROPERTY(EditAnywhere, Category = "CoreKit|Navigation")
	TMap<FKey, FCoreKitKeyPresentation> Keys;
};
