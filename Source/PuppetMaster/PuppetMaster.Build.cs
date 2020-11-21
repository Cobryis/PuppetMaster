// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PuppetMaster : ModuleRules
{
	public PuppetMaster(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", "CoreUObject", "Engine", "InputCore",
			"NavigationSystem", "AIModule",
			"GameplayAbilities", "GameplayTasks", "GameplayTags",
		});
    }
}
