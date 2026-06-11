using UnrealBuildTool;

public class CoreGlue : ModuleRules
{
    public CoreGlue(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                "CoreGlue/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "CoreGlue/Private"
            }
        );
    }
}
