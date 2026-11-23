using UnrealBuildTool;

public class Delerict : ModuleRules
{
    public Delerict(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "AnimGraphRuntime", "CoreKitTween",
            "EnhancedInput", "GameplayCameras", "InputCore", "Mover"
        });
    }
}
