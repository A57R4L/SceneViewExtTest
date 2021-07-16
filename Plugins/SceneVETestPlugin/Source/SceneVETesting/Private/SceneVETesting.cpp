// Test project fo SceneViewExtension / RDG Shader Basic setup
// Copyright 2021 Ossi Luoto
// 
// Main Module to set Shader Directories

#include "SceneVETesting.h"

#define LOCTEXT_NAMESPACE "FSceneVETesting"

void FSceneVETesting::StartupModule()
{
// Set up the Shader Directories
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("SceneVETestPlugin"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugins/SceneVETestPlugin"), PluginShaderDir);
}

void FSceneVETesting::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSceneVETesting, SceneVETesting);