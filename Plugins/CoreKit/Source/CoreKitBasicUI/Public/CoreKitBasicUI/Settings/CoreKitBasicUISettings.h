#pragma once

#include "Engine/DeveloperSettings.h"
#include "CoreKitBasicUISettings.generated.h"

class UCoreKitBasicUIPresenter;
class UInputAction;
class UInputMappingContext;
class USoundBase;
class UWorld;

/** Configuration owned by the consuming project. BasicUI never discovers gameplay classes. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="CoreKit Basic UI"))
class COREKITBASICUI_API UCoreKitBasicUISettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    FName GetCategoryName() const override { return TEXT("Game"); }
    /** Enables local UI contexts and settings application. Leave off until the consumer opts in. */
    UPROPERTY(Config, EditAnywhere, Category="Presentation") bool bEnabled = false;
    UPROPERTY(Config, EditAnywhere, Category="Branding") FText TitleFirst = FText::FromString(TEXT("CORE"));
    UPROPERTY(Config, EditAnywhere, Category="Branding") FText TitleSecond = FText::FromString(TEXT("KIT"));
    UPROPERTY(Config, EditAnywhere, Category="Branding") FText Tagline = FText::FromString(TEXT("YOUR NEXT RUN STARTS HERE"));
    UPROPERTY(Config, EditAnywhere, Category="Branding") FText TeamName = FText::FromString(TEXT("YOUR TEAM"));
    UPROPERTY(Config, EditAnywhere, Category="Branding") FText LoadingLabel = FText::FromString(TEXT("LOADING"));
    /** Overrides inherited labels/help/credits by Namespace.Key (see Docs/BasicUITextKeys.json). */
    UPROPERTY(Config, EditAnywhere, Category="Branding") TMap<FName, FText> TextOverrides;
    UPROPERTY(Config, EditAnywhere, Category="Presentation", meta=(ClampMin="1")) float DefaultSessionSeconds = 240.f;
    UPROPERTY(Config, EditAnywhere, Category="Presentation", meta=(ClampMin="0")) int32 ExampleHealAmount = 25;
    UPROPERTY(Config, EditAnywhere, Category="Presentation") TArray<TSubclassOf<UCoreKitBasicUIPresenter>> PresenterClasses;
    /** Models are always local-player owned. Enable indexed aliases only if MVVM collection bindings need them. */
    UPROPERTY(Config, EditAnywhere, Category="Presentation") bool bRegisterGlobalViewModels = false;
    UPROPERTY(Config, EditAnywhere, Category="Presentation") bool bEnableCameraEffects = false;
    UPROPERTY(Config, EditAnywhere, Category="Presentation") bool bEnableWorldSignals = false;
    /** Used only by the standalone menu fallback. CoreKitFrontend owns travel when enabled. */
    UPROPERTY(Config, EditAnywhere, Category="Travel") TSoftObjectPtr<UWorld> GameplayMap;
    UPROPERTY(Config, EditAnywhere, Category="Input") TSoftObjectPtr<UInputMappingContext> InputMapping;
    /** Keys are UI roles such as IA_Jump, IA_Dash, IA_Reload and IA_InspectBuild. Values belong to your game. */
    UPROPERTY(Config, EditAnywhere, Category="Input") TMap<FName, TSoftObjectPtr<UInputAction>> InputActions;
    UPROPERTY(Config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> MenuMusic;
    UInputAction* FindAction(FName Role) const;
    static FText ResolveText(FName Key, const FText& Fallback);
};

#define COREKIT_BASICUI_TEXT(Key, Default) UCoreKitBasicUISettings::ResolveText(FName(*(FString(TEXT(LOCTEXT_NAMESPACE)) + TEXT(".") + TEXT(Key))), LOCTEXT(Key, Default))
