using UnrealBuildTool;

public class WorksOnMyMachineTarget : TargetRules
{
    public WorksOnMyMachineTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("WorksOnMyMachine");
    }
}
