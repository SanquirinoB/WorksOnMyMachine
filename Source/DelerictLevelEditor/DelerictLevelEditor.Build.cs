using UnrealBuildTool;

public class DelerictLevelEditor : ModuleRules
{
    public DelerictLevelEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "Delerict", "UnrealEd", "PlacementMode",
            "AssetRegistry", "AssetTools", "BlueprintGraph", "Kismet", "KismetCompiler", "SlateCore"
        });
    }
}
