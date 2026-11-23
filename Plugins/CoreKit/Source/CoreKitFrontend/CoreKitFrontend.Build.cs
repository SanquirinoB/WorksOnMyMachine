using UnrealBuildTool;

public class CoreKitFrontend : ModuleRules
{
	public CoreKitFrontend(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DeveloperSettings", "InputCore", "UMG", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new[] { "CoreKitLoading", "CoreKitSave", "CoreKitNavigation", "EnhancedInput", "Slate" });
	}
}
