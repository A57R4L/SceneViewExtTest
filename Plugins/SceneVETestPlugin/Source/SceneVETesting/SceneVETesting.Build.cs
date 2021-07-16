// Test project fo SceneViewExtension / RDG Shader Basic setup
// Copyright 2021 Ossi Luoto
// 
// Build tool Dependencies
// IMPORTANT! Paths required for Build tool/Visual Studio to locate f.ex. Screenpass.h and Post Process stuff

using UnrealBuildTool;
using System.IO;

public class SceneVETesting : ModuleRules
{
    public SceneVETesting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "Renderer",
            "RHI",
            "Projects",
            });

        var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

        PrivateIncludePaths.AddRange(
            new string[] {
				// this is required to find PostProcessing includes f.ex. screenpass.h
				Path.Combine(EngineDir, "Source/Runtime/Renderer/Private")
            }
        );

    }
}
