using UnrealBuildTool;
public class PieceOfCakeEditorTarget : TargetRules
{
    public PieceOfCakeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PieceOfCake");
    }
}
