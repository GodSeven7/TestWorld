using UnrealBuildTool;

public class CombatSystemPlugin : ModuleRules
{
    public CombatSystemPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "CoreGlue"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                "CombatSystemPlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "CombatSystemPlugin/Private"
            }
        );
    }
}
