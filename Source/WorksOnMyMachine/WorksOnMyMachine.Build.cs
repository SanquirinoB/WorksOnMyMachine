using UnrealBuildTool;

public class WorksOnMyMachine : ModuleRules
{
    public WorksOnMyMachine(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "EnhancedInput", "InputCore", "Mover"
        });
    }
}
