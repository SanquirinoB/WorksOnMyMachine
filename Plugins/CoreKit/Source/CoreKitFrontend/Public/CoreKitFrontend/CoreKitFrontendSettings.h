#pragma once

#include "Engine/DeveloperSettings.h"
#include "CoreKitFrontendSettings.generated.h"

class UCoreKitFrontendMenu;
class UInputMappingContext;
class USoundClass;
class USoundMix;
class UUserWidget;
class UWorld;

USTRUCT()
struct FCoreKitFrontendBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Controls")
	FText Label;

	UPROPERTY(EditAnywhere, Category = "Controls")
	TSoftObjectPtr<UInputMappingContext> Context;

	UPROPERTY(EditAnywhere, Category = "Controls")
	FName MappingName;

	/** Enhanced Input slot, starting at zero. */
	UPROPERTY(EditAnywhere, Category = "Controls", meta = (ClampMin = "0", ClampMax = "6"))
	int32 Slot = 0;

	UPROPERTY(EditAnywhere, Category = "Controls")
	FName HardwareDeviceId;

	UPROPERTY(EditAnywhere, Category = "Controls")
	bool bGamepad = false;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CoreKit Frontend"))
class COREKITFRONTEND_API UCoreKitFrontendSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Startup")
	bool bEnabled = false;

	UPROPERTY(Config, EditAnywhere, Category = "Startup")
	TSoftObjectPtr<UWorld> MainMenuMap;

	UPROPERTY(Config, EditAnywhere, Category = "Startup")
	TSoftObjectPtr<UWorld> FirstGameMap;

	/** Leave empty to use the built-in menu. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation")
	TSubclassOf<UCoreKitFrontendMenu> MenuWidgetClass;

	/** Leave empty to use the built-in loading screen. */
	UPROPERTY(Config, EditAnywhere, Category = "Presentation")
	TSubclassOf<UUserWidget> LoadingWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category = "Travel")
	bool bRequirePossessedPawn = false;

	/** A game that streams its entry area can signal readiness explicitly before restore. */
	UPROPERTY(Config, EditAnywhere, Category = "Travel")
	bool bWaitForProjectReady = false;

	UPROPERTY(Config, EditAnywhere, Category = "Travel", meta = (ClampMin = "1.0"))
	float TravelTimeoutSeconds = 30.f;

	UPROPERTY(Config, EditAnywhere, Category = "Save")
	FString SlotPrefix = TEXT("corekit");

	UPROPERTY(Config, EditAnywhere, Category = "Save")
	bool bAllowMissingParticipants = true;

	UPROPERTY(Config, EditAnywhere, Category = "Options|Audio")
	TSoftObjectPtr<USoundMix> UserVolumeMix;

	UPROPERTY(Config, EditAnywhere, Category = "Options|Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Options|Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Options|Audio")
	TSoftObjectPtr<USoundClass> EffectsSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Options|Controls", meta = (TitleProperty = "Label"))
	TArray<FCoreKitFrontendBinding> EditableBindings;

	UPROPERTY(Config, EditAnywhere, Category = "Options|Controls")
	TArray<TSoftObjectPtr<UInputMappingContext>> ConcurrentBindingContexts;
};
