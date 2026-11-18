using UnrealBuildTool;
using System.Collections.Generic;

public class RoundTarget : TargetRules
{
    public RoundTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        ExtraModuleNames.Add("Round");
    }
}
