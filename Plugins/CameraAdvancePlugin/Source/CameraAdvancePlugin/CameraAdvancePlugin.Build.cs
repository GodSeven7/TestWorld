using UnrealBuildTool;

public class CameraAdvancePlugin : ModuleRules
{
    public CameraAdvancePlugin(ReadOnlyTargetRules Target) : base(Target)
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
                "CoreGlue"
            }
        );

        PublicIncludePaths.AddRange(
            new string[]
            {
                "CameraAdvancePlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "CameraAdvancePlugin/Private"
            }
        );
    }
}
