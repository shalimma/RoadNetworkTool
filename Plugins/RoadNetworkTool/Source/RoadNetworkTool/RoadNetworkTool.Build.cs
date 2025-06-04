// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RoadNetworkTool : ModuleRules
{
	public RoadNetworkTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;	
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput"
				// ... add other public dependencies that you statically link with here ...
			}
			);
		
	}
}
