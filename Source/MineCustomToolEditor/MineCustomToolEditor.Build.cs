// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MineCustomToolEditor : ModuleRules
{

	public MineCustomToolEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		// this.bEnableExceptions = true;
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "Engine",
                "CoreUObject",
                "InputCore",
                "LevelEditor",
                "Slate",
                "EditorStyle",
                "AssetTools",
                "EditorWidgets",
                "UnrealEd",
                "BlueprintGraph",
                "AnimGraph",
                "ComponentVisualizers",
                // ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Engine",
                "AppFramework",
                "SlateCore",
                "AnimGraph",
                "UnrealEd",
                "KismetWidgets",
                "MainFrame",
                "PropertyEditor",
                "ComponentVisualizers",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);


	}
}
