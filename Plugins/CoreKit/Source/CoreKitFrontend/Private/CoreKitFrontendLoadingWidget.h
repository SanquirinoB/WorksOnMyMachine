#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreKitFrontendLoadingWidget.generated.h"

UCLASS()
class UCoreKitFrontendLoadingWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	TSharedRef<SWidget> RebuildWidget() override;
};
