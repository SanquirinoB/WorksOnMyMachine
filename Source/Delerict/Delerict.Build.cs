using UnrealBuildTool;

public class Delerict : ModuleRules
{
    public Delerict(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDefinitions.Add(Target.Configuration == UnrealTargetConfiguration.Development
            ? "DELERICT_WITH_GAMEPLAY_DEBUG=1"
            : "DELERICT_WITH_GAMEPLAY_DEBUG=0");
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "AnimGraphRuntime", "CoreKitTween",
            "CoreKitBasicUI", "CoreKitFrontend", "CoreKitNavigation",
            "EnhancedInput", "GameplayCameras", "GameplayTags", "InputCore", "Mover", "Slate", "SlateCore", "UMG"
        });
    }
}
