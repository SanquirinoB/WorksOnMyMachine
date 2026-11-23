using UnrealBuildTool;

public class CoreKitExample : ModuleRules
{
	public CoreKitExample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "GameplayTasks",
			"UMG", "Slate", "SlateCore", "CoreKitData", "CoreKitSave", "CoreKitFrontend", "CoreKitNavigation",
			"CoreKitTween", "CoreKitActions"
		});
	}
}
