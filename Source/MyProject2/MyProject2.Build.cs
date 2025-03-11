// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject2 : ModuleRules
{
	public MyProject2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
		// Public 의존성: 다른 모듈에서도 접근 가능하게 해야 하는 모듈
		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"ApplicationCore", 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"EnhancedInput", 
			"Slate", 
			"SlateCore", 
			"InputCore", 
		});

		// Private 의존성: 이 모듈 내에서만 사용되는 모듈
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"UnrealEd",
			"DatasmithCore",
			"DatasmithContent",
			"DatasmithImporter"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}