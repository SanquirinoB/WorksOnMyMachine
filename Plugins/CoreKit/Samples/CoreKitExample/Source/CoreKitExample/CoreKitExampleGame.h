#pragma once

#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "CoreKitExampleGame.generated.h"

class UButton;
class UCoreKitExampleBlockAction;
class UCoreKitNavigationGlyphs;
class UInputAction;
class UInputMappingContext;
class UTextBlock;

UCLASS(Blueprintable)
class UCoreKitExampleLoadingWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	TSharedRef<SWidget> RebuildWidget() override;
};

UCLASS(Blueprintable)
class UCoreKitExamplePanel : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatus(const FText& Message);
	UButton* GetInitialButton() const { return InitialButton; }

protected:
	TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UButton> InitialButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Status;
};

UCLASS(Blueprintable)
class UCoreKitExampleGame : public UGameInstance
{
	GENERATED_BODY()

public:
	void OnStart() override;

	UInputAction* GetDoorInput() const { return DoorInput; }
	UInputAction* GetBlockInput() const { return BlockInput; }
	UCoreKitNavigationGlyphs* GetGlyphs() const { return Glyphs; }
	TSubclassOf<UCoreKitExampleBlockAction> GetBlockActionClass() const;
	float GetDoorDuration() const { return DoorDuration; }
	void ShowBlockStatus();

	UFUNCTION(BlueprintCallable, Category = "Example")
	void BackToMenu();
	UFUNCTION(BlueprintCallable, Category = "Example")
	void ToggleDoor();
	UFUNCTION(BlueprintCallable, Category = "Example")
	void SaveCheckpoint();
	UFUNCTION(BlueprintCallable, Category = "Example")
	void PressBlock();
	UFUNCTION(BlueprintCallable, Category = "Example")
	void ReleaseBlock();
	UFUNCTION(BlueprintCallable, Category = "Example")
	void SimulateBusy();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Example")
	TSubclassOf<UCoreKitExamplePanel> PanelClass;
	UPROPERTY(EditDefaultsOnly, Category = "Example")
	TSubclassOf<UCoreKitExampleBlockAction> BlockActionClass;
	UPROPERTY(EditDefaultsOnly, Category = "Example")
	TObjectPtr<UCoreKitNavigationGlyphs> Glyphs;
	UPROPERTY(EditDefaultsOnly, Category = "Example")
	TSoftObjectPtr<UWorld> PlaygroundMap;
	UPROPERTY(EditDefaultsOnly, Category = "Example|Input")
	TObjectPtr<UInputAction> DoorInput;
	UPROPERTY(EditDefaultsOnly, Category = "Example|Input")
	TObjectPtr<UInputAction> BlockInput;
	UPROPERTY(EditDefaultsOnly, Category = "Example|Input")
	TObjectPtr<UInputMappingContext> InputContext;

private:
	void ShowPlayground(const FText& Message = {});
	void ClosePage();
	void ShowStatus(const FText& Message);
	void RegisterInput();
	void LoadInteractionCatalog();
	bool IsPlayground() const;

	UFUNCTION()
	void GameArrived(bool bContinuing);
	UFUNCTION()
	void FrontendChanged();
	UFUNCTION()
	void CheckpointSaved(bool bSucceeded);
	UFUNCTION()
	FText GetBlockStatus() const;

	UPROPERTY(Transient)
	TObjectPtr<UCoreKitExamplePanel> Panel;

	FGuid FocusScope;
	float DoorDuration = .45f;
};
