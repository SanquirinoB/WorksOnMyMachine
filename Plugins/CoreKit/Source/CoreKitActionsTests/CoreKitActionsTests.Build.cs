using UnrealBuildTool;

public class CoreKitActionsTests : ModuleRules
{
	public CoreKitActionsTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"GameplayTasks",
				"Projects",
				"CoreKitActions"
			}
		);
	}
}
