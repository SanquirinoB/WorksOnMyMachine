using UnrealBuildTool;

public class CoreKitWeapons : ModuleRules
{
    public CoreKitWeapons(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayTags", "CoreKitActions", "GameplayTasks" });
    }
}
