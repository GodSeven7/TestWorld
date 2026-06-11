// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestWorld : ModuleRules
{
    public TestWorld(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore", 
			"EnhancedInput", 
			"CinematicCamera",
			"CoreGlue"
		});

        PrivateDependencyModuleNames.AddRange(new string[] {
            "SparseGridPlugin",
            "ObjectPoolPlugin",
			"GOAP",
            "BattleDamagePlugin",
            "CombatSystemPlugin",
            "ProjectilePlugin",
            "QuestPlugin",
            "GameUIPlugin",
            "CharacterInstancingPlugin",
            "CameraAdvancePlugin",
            "UMG",
            "Slate",
            "SlateCore"
		});
    }
}
