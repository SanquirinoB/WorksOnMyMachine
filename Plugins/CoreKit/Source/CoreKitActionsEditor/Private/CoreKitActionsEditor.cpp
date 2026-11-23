#include "CoreKitActions/Actions/Components/CoreKitActionComponent.h"

#include "CoreKitActions/Actions/CoreKitAction.h"
#include "GameFramework/PlayerController.h"
#include "GameplayDebugger.h"
#include "GameplayDebuggerCategory.h"
#include "Modules/ModuleManager.h"

namespace
{
const FName CategoryName(TEXT("CoreKitActions"));

class FCoreKitActionsCategory final : public FGameplayDebuggerCategory
{
public:
	FCoreKitActionsCategory()
	{
		bShowOnlyWithDebugActor = false;
	}

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance()
	{
		return MakeShared<FCoreKitActionsCategory>();
	}

	void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override
	{
		AActor* Observed = DebugActor ? DebugActor : (OwnerPC ? OwnerPC->GetPawn() : nullptr);
		const auto* Actions = Observed ? Observed->FindComponentByClass<UCoreKitActionComponent>() : nullptr;
		if (!DebugActor && !Actions && OwnerPC)
		{
			Observed = OwnerPC;
			Actions = OwnerPC->FindComponentByClass<UCoreKitActionComponent>();
		}
		if (!Actions)
		{
			AddTextLine(TEXT("Select an actor with a Core Kit Action Component."));
			return;
		}

		AddTextLine(FString::Printf(TEXT("Actor: %s   IASA: %s"), *GetNameSafe(Observed), Actions->IsIASA() ? TEXT("open") : TEXT("closed")));
		for (const UCoreKitAction* Action : Actions->GetActiveActions())
		{
			if (IsValid(Action))
			{
				AddTextLine(FString::Printf(TEXT("Active: %s (%s)"), *Action->GetActionTag().ToString(), *Action->GetExecutionId().ToString()));
			}
		}
		if (Actions->GetQueuedRequestId().IsValid())
		{
			AddTextLine(FString::Printf(TEXT("Queued: %s (%s)"), *Actions->GetQueuedActionTag().ToString(), *Actions->GetQueuedRequestId().ToString()));
		}
		for (const FString& Block : Actions->DescribeIASABlocks())
		{
			AddTextLine(TEXT("IASA owner: ") + Block);
		}

		const auto History = Actions->GetRequestHistory();
		for (int32 Index = History.Num() - 1; Index >= FMath::Max(0, History.Num() - 4); --Index)
		{
			const auto& Report = History[Index];
			AddTextLine(FString::Printf(TEXT("Request: %s | %s | %s %s"), *Report.ActionTag.ToString(),
				*UEnum::GetValueAsString(Report.Status), *UEnum::GetValueAsString(Report.Result), *Report.Detail));
		}
	}
};
}

class FCoreKitActionsEditorModule final : public IModuleInterface
{
	void StartupModule() override
	{
		auto& Debugger = IGameplayDebugger::Get();
		Debugger.RegisterCategory(CategoryName, IGameplayDebugger::FOnGetCategory::CreateStatic(&FCoreKitActionsCategory::MakeInstance),
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
		Debugger.NotifyCategoriesChanged();
	}

	void ShutdownModule() override
	{
		if (IGameplayDebugger::IsAvailable())
		{
			auto& Debugger = IGameplayDebugger::Get();
			Debugger.UnregisterCategory(CategoryName);
			Debugger.NotifyCategoriesChanged();
		}
	}
};

IMPLEMENT_MODULE(FCoreKitActionsEditorModule, CoreKitActionsEditor)
