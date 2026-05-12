// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LastRefuge : ModuleRules
{
	public LastRefuge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem", 
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"LastRefuge",
			"LastRefuge/Variant_Horror",
			"LastRefuge/Variant_Horror/UI",
			"LastRefuge/Variant_Shooter",
			"LastRefuge/Variant_Shooter/AI",
			"LastRefuge/Variant_Shooter/UI",
			"LastRefuge/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
