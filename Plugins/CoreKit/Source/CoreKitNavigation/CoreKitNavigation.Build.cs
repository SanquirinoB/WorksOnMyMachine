using UnrealBuildTool;

public class CoreKitNavigation : ModuleRules
{
	public CoreKitNavigation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "EnhancedInput", "DeveloperSettings" });
		PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "GameplayTags" });
	}
}
