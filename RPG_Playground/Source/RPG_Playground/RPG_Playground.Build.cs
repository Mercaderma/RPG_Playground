// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RPG_Playground : ModuleRules
{
	public RPG_Playground(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RPG_Playground",
			"RPG_Playground/Variant_Platforming",
			"RPG_Playground/Variant_Platforming/Animation",
			"RPG_Playground/Variant_Combat",
			"RPG_Playground/Variant_Combat/AI",
			"RPG_Playground/Variant_Combat/Animation",
			"RPG_Playground/Variant_Combat/Gameplay",
			"RPG_Playground/Variant_Combat/Interfaces",
			"RPG_Playground/Variant_Combat/UI",
			"RPG_Playground/Variant_SideScrolling",
			"RPG_Playground/Variant_SideScrolling/AI",
			"RPG_Playground/Variant_SideScrolling/Gameplay",
			"RPG_Playground/Variant_SideScrolling/Interfaces",
			"RPG_Playground/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
