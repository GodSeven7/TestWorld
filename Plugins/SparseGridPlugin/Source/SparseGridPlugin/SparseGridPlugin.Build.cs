using UnrealBuildTool;
using System.IO;

public class SparseGridPlugin : ModuleRules
{
    public SparseGridPlugin(ReadOnlyTargetRules Target) : base(Target)
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
                Path.Combine(ModuleDirectory, "Public/GridCore"),
                Path.Combine(ModuleDirectory, "Public/Collision"),
                Path.Combine(ModuleDirectory, "Public/Query"),
                Path.Combine(ModuleDirectory, "Public/Navigation"),
                Path.Combine(ModuleDirectory, "Public/Steering"),
                Path.Combine(ModuleDirectory, "Public/CrowdSurround"),
                Path.Combine(ModuleDirectory, "Public/Debug")
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                Path.Combine(ModuleDirectory, "Private/GridCore"),
                Path.Combine(ModuleDirectory, "Private/Collision"),
                Path.Combine(ModuleDirectory, "Private/Query"),
                Path.Combine(ModuleDirectory, "Private/Navigation"),
                Path.Combine(ModuleDirectory, "Private/CrowdSurround"),
                Path.Combine(ModuleDirectory, "Private/Debug")
            }
        );
    }
}
