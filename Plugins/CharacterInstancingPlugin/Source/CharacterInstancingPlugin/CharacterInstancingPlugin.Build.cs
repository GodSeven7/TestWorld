using UnrealBuildTool;

public class CharacterInstancingPlugin : ModuleRules
{
    public CharacterInstancingPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bValidateExperimentalApi = false;
        bTreatAsEngineModule = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "RenderCore",
                "AnimationCore",
                "CoreGlue"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                "CharacterInstancingPlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "CharacterInstancingPlugin/Private"
            }
        );
    }
}
