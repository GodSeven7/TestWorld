// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ObjectPoolPlugin : ModuleRules
{
	public ObjectPoolPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"CoreGlue",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
			}
		);

		PublicIncludePaths.AddRange(
			new string[] {
				"ObjectPoolPlugin/Public"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"ObjectPoolPlugin/Private"
			}
		);
	}
}
