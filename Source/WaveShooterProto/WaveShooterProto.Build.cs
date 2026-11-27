// Copyright Acolyte Games. All Rights Reserved.

using UnrealBuildTool;

public class WaveShooterProto : ModuleRules
{
	public WaveShooterProto(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "NetCore", "AnimGraphRuntime", "UMG", "Slate", "SlateCore" });
	}
}
