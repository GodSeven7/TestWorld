using UnrealBuildTool;

public class QuestPlugin : ModuleRules
{
    public QuestPlugin(ReadOnlyTargetRules Target) : base(Target)
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
                "QuestPlugin/Public"
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                "QuestPlugin/Private"
            }
        );
    }
}
