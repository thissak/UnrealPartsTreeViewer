using UnrealBuildTool;

public class MyProject2Editor : ModuleRules
{
    public MyProject2Editor(ReadOnlyTargetRules Target) : base(Target)
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
                "ToolMenus",
                "MyProject2",
                "DatasmithCore",         // 추가
                "DatasmithContent",      // 추가
                "AssetTools"             // AssetImportTask를 위해 추가
            }
        );
    }
}