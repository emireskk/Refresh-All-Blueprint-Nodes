using UnrealBuildTool;

public class RefreshAllNodes : ModuleRules
{
    public RefreshAllNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "Kismet",
                "KismetCompiler",
                "BlueprintGraph",
                "AssetRegistry",
                "AssetTools",
                "EditorStyle",
                "ToolMenus",
                "Projects",
                "ContentBrowser"
            }
        );
    }
}