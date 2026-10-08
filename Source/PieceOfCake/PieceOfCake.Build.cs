using UnrealBuildTool;
public class PieceOfCake : ModuleRules
{
    public PieceOfCake(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "HTTP", "InputCore", "Json", "JsonUtilities",
            "UMG", "Slate", "SlateCore", "RenderCore", "PixelStreaming2", "PixelStreaming2Input"
        });
        if (Target.Platform == UnrealTargetPlatform.Mac) PublicFrameworks.AddRange(new[] { "Security", "CoreFoundation" });
    }
}
