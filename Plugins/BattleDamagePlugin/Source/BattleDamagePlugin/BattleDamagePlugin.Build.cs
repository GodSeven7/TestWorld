using UnrealBuildTool;

public class BattleDamagePlugin : ModuleRules
{
    public BattleDamagePlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Niagara",
                "CoreGlue"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                "BattleDamagePlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "BattleDamagePlugin/Private"
            }
        );
    }
}
