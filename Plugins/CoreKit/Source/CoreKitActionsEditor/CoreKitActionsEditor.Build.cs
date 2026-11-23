using UnrealBuildTool;

public class CoreKitActionsEditor : ModuleRules
{
	public CoreKitActionsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayDebugger", "CoreKitActions" });
	}
}
