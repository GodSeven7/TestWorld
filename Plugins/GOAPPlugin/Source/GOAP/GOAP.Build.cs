using UnrealBuildTool;

public class GOAP : ModuleRules
{
    public GOAP(ReadOnlyTargetRules Target) : base(Target)
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
                "GOAP/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "GOAP/Private"
            }
        );
    }
}
