using UnrealBuildTool;

public class CoreKitExampleEditorTarget : TargetRules
{
	public CoreKitExampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CoreKitExample");
	}
}
