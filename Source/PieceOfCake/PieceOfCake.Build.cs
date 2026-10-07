using UnrealBuildTool;
public class PieceOfCake : ModuleRules
{
    public PieceOfCake(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities",
            "UMG", "Slate", "SlateCore", "PixelStreaming2", "PixelStreaming2Input"
        });
    }
}
