using UnrealBuildTool;
public class CoreKitBasicUI : ModuleRules
{
    public CoreKitBasicUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "ModelViewViewModel", "FieldNotification", "GameplayTags", "DeveloperSettings", "CoreKitNavigation" });
        PrivateDependencyModuleNames.AddRange(new[] { "CoreKitFrontend", "CoreKitLoading", "CoreKitSave", "RenderCore", "AudioMixer", "AssetRegistry", "EngineSettings" });
        RuntimeDependencies.Add("$(PluginDir)/Resources/BasicUI/ThirdParty/...", StagedFileType.NonUFS);
        PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Private"));
    }
}
