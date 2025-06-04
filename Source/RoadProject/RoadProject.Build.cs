// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RoadProject : ModuleRules
{
	public RoadProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"UMG"
		});

		PublicIncludePaths.AddRange(new string[] {
			"RoadProject",
			"RoadProject/SportsCar",
			"RoadProject/OffroadCar",
			"RoadProject/Variant_Offroad",
			"RoadProject/Variant_TimeTrial"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
