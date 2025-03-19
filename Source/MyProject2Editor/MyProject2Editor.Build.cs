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
				"EditorStyle",    // 추가: 에디터 스타일 모듈
				"UnrealEd",       // 추가: 에디터 핵심 모듈 (GEditor 포함)
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
				"AssetTools", 
				"WorkspaceMenuStructure",
				"InputCore",             // 추가: 입력 처리 모듈
				"LevelEditor",           // 추가: 레벨 에디터 모듈
			}
		);
	}
}