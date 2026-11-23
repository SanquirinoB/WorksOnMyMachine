using UnrealBuildTool;

public class CoreKitLoading : ModuleRules
{
	public CoreKitLoading(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] { "UMG", "Slate", "SlateCore" });
	}
}
