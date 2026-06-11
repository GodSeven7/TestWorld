using UnrealBuildTool;

public class ProjectilePlugin : ModuleRules
{
    public ProjectilePlugin(ReadOnlyTargetRules Target) : base(Target)
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
                "ProjectilePlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "ProjectilePlugin/Private"
            }
        );
    }
}
