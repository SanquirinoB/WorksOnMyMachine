using UnrealBuildTool;

public class CoreKitData : ModuleRules
{
	public CoreKitData(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayTags" });
		PrivateDependencyModuleNames.Add("AssetRegistry");
	}
}
