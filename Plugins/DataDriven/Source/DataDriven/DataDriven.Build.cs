using UnrealBuildTool;

public class DataDriven : ModuleRules
{
	public DataDriven(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"AssetRegistry"
			}
		);
	}
}
