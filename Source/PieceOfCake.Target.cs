using UnrealBuildTool;
public class PieceOfCakeTarget : TargetRules
{
    public PieceOfCakeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PieceOfCake");
    }
}
