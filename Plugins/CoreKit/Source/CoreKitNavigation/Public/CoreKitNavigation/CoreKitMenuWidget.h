#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreKitMenuWidget.generated.h"

class UCoreKitNavigationSubsystem;
class UCoreKitSectionList;
class UWidgetAnimation;

/** How a menu at the top of the stack drives the player's input mode. */
UENUM(BlueprintType)
enum class ECoreKitMenuInputMode : uint8
{
	/** Input stays with the game (HUD-like menus). */
	Game,
	/** Game and UI both receive input. */
	GameAndMenu,
	/** Only UI receives input. */
	Menu
};

/**
 * Base class for menus managed by UCoreKitNavigationSubsystem (PushMenu/PopMenu).
 * The top menu owns focus, input mode, cursor, Return and section input. Optional animations named
 * Anim_In and Anim_Out play on activation and before removal.
 */
UCLASS(Abstract, meta = (DisplayName = "Core Kit Menu Widget"))
class COREKITNAVIGATION_API UCoreKitMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Asks the owning subsystem to pop this menu (plays Anim_Out when present). */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void CloseMenu();

	/** True while the activation cooldown blocks Select/Return. */
	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	bool IsInputCoolingDown() const;

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	bool IsActiveMenu() const { return bActiveMenu; }

	UFUNCTION(BlueprintPure, Category = "CoreKit|Navigation")
	FName GetMenuLayer() const { return MenuLayer; }

	/** Widget that receives focus when the menu becomes the top menu. Defaults to the first focusable widget. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Navigation")
	UWidget* GetInitialFocus() const;

	/** Return/Back. Returns true when handled. Default: close the menu when bAllowReturn. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Navigation")
	bool HandleReturn();

	/** Next (+1) or previous (-1) section. Default: forwards to the menu's section list. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Navigation")
	bool HandleSection(int32 Delta);

	/** Any other key while this is the top menu. Return true to consume it. */
	UFUNCTION(BlueprintNativeEvent, Category = "CoreKit|Navigation")
	bool HandleMenuKey(FKey Key);

	/** Registers the section list that HandleSection drives. Found automatically when left unset. */
	UFUNCTION(BlueprintCallable, Category = "CoreKit|Navigation")
	void SetSectionList(UCoreKitSectionList* InSections);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	ECoreKitMenuInputMode InputMode = ECoreKitMenuInputMode::Menu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	bool bShowCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	bool bAllowReturn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	bool bAllowSections = true;

	/** Seconds after activation during which Select and Return are ignored. Negative uses the project default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoreKit|Navigation")
	float ActivationInputCooldown = -1.f;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit|Navigation")
	void OnMenuActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "CoreKit|Navigation")
	void OnMenuDeactivated();

	virtual void NativeOnMenuActivated();
	virtual void NativeOnMenuDeactivated();

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_In;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "CoreKit|Navigation", meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> Anim_Out;

private:
	friend class UCoreKitNavigationSubsystem;

	void SetMenuActive(bool bActive);
	void BeginRemoval();

	UFUNCTION()
	void HandleOutFinished();

	UCoreKitSectionList* FindSectionList() const;

	TWeakObjectPtr<UCoreKitNavigationSubsystem> Owner;
	TWeakObjectPtr<UCoreKitSectionList> Sections;
	FName MenuLayer;
	double ActivatedAt = 0.0;
	bool bActiveMenu = false;
	bool bRemoving = false;
};
