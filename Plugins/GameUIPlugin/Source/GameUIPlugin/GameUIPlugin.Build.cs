using UnrealBuildTool;

public class GameUIPlugin : ModuleRules
{
    public GameUIPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UMG",
                "Slate",
                "SlateCore",
                "CoreGlue"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                "GameUIPlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "GameUIPlugin/Private"
            }
        );
    }
}
