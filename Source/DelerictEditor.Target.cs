using UnrealBuildTool;

public class DelerictEditorTarget : TargetRules
{
    public DelerictEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("Delerict");
        ExtraModuleNames.Add("DelerictLevelEditor");
    }
}
