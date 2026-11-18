using System.IO;
using UnrealBuildTool;

public class RoundCore : ModuleRules
{
    public RoundCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
        PublicSystemIncludePaths.Add(Path.Combine(ProjectRoot, "RoundCore", "include"));

        // Required only by the minimal Unreal module entry point. The gameplay
        // implementation itself remains standard C++ and has no Unreal includes.
        PrivateDependencyModuleNames.Add("Core");
    }
}
