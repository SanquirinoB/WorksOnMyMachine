#include "Modules/ModuleManager.h"
#include "GameplayTagsManager.h"
#include "Interfaces/IPluginManager.h"

class FCoreKitActionsTestsModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CoreKit"));
		if (ensureMsgf(Plugin.IsValid(), TEXT("CoreKitActionsTests could not locate the CoreKit plugin.")))
		{
			TestTagConfigPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Tests/CoreKitActions/Config/Tags"));
			if (ensureMsgf(FPaths::DirectoryExists(TestTagConfigPath),
				TEXT("CoreKitActionsTests could not find tag data at '%s'."), *TestTagConfigPath))
			{
				UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
				TagsManager.AddTagIniSearchPath(TestTagConfigPath);
				ensureMsgf(TagsManager.RequestGameplayTag(TEXT("CoreKitActions.Tests.Action.A"), false).IsValid(),
					TEXT("CoreKitActionsTests did not load its Gameplay Tag configuration from '%s'."),
					*TestTagConfigPath);
			}
		}
	}

	virtual void ShutdownModule() override
	{
		if (!TestTagConfigPath.IsEmpty() && UGameplayTagsManager::GetIfAllocated() != nullptr)
		{
			UGameplayTagsManager::GetIfAllocated()->RemoveTagIniSearchPath(TestTagConfigPath);
		}
	}

private:
	FString TestTagConfigPath;
};

IMPLEMENT_MODULE(FCoreKitActionsTestsModule, CoreKitActionsTests)
