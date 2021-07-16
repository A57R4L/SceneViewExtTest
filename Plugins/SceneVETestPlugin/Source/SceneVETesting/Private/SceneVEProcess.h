// Test project fo SceneExtensionView / RDG Shader Basic setup
// Copyright 2021 Ossi Luoto
// 
// SceneVEProcess.cpp - Actual RDG hook and Shader Loading

#pragma once
#include "SceneVEComponent.h"

#include "ScreenPass.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "ShaderParameterStruct.h"

#include "RenderGraphUtils.h"

// Parameter Declaration
BEGIN_SHADER_PARAMETER_STRUCT(FSceneVETestShaderParameters, )
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
	SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

// Declare Test Shader Class

class SCENEVETESTING_API FSceneVETestShaderVS : public FGlobalShader
{
public:
	// Vertex Shader Declaration
	DECLARE_GLOBAL_SHADER(FSceneVETestShaderVS)

	// Basic shader stuff
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	FSceneVETestShaderVS() {}

	FSceneVETestShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}
};

class SCENEVETESTING_API FSceneVETestShaderPS : public FGlobalShader
{
public:
	// RDG Pixel Shader Declaration
	DECLARE_GLOBAL_SHADER(FSceneVETestShaderPS)
	SHADER_USE_PARAMETER_STRUCT_WITH_LEGACY_BASE(FSceneVETestShaderPS, FGlobalShader)

	// Basic shader stuff
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	// Use the parameters from previously delcared struct
	using FParameters = FSceneVETestShaderParameters;

};

class FSceneVEProcess
{
public:
	// Hook to the SceneViewExtension Base
	static FScreenPassTexture AddSceneVETestPass(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

private:

};