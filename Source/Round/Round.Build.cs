using System.IO;
using UnrealBuildTool;

public class Round : ModuleRules
{
    public Round(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;
        bUseRTTI = true;

        string ProjectRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
        string PybindIncludePath = Path.Combine(ProjectRoot, "pybind11-master", "include");
        string PythonRoot = @"C:\Users\marco\AppData\Local\Programs\Python\Python313";
        string PythonIncludePath = Path.Combine(PythonRoot, "include");
        string PythonLibPath = Path.Combine(PythonRoot, "libs", "python313.lib");
        string PythonDllPath = Path.Combine(PythonRoot, "python313.dll");

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",
            "NetCore",
            "AssetRegistry",
            "Json",
            "JsonUtilities",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "RoundCore"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "UMGEditor", "Kismet" });
        }

        PublicSystemIncludePaths.Add(PybindIncludePath);
        PublicSystemIncludePaths.Add(PythonIncludePath);
        PublicAdditionalLibraries.Add(PythonLibPath);
        RuntimeDependencies.Add(Path.Combine(ProjectRoot, "Binaries", "Win64", "python313.dll"), PythonDllPath);

    }
}
