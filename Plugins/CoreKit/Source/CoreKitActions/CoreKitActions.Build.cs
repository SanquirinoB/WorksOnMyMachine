using UnrealBuildTool;

public class CoreKitActions : ModuleRules
{
	public CoreKitActions(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"GameplayTasks"
			}
		);
	}
}
