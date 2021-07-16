// Test project fo SceneViewExtension / RDG Shader Basic setup
// Copyright 2021 Ossi Luoto
// 
// SceneVEProcess.cpp - Actual RDG hook and Shader Loading

#include "SceneVEProcess.h"

// Shader implementation Macro doesn't work on .h file so load them here
IMPLEMENT_GLOBAL_SHADER(FSceneVETestShaderPS, "/Plugins/SceneVETestPlugin/SceneVETestShaderPS.usf", "MainPS", SF_Pixel);    // point to the shader  file, name of the main function in shader.usf
IMPLEMENT_GLOBAL_SHADER(FSceneVETestShaderVS, "/Plugins/SceneVETestPlugin/SceneVETestShaderVS.usf", "MainVS", SF_Vertex);    // point to the shader  file, name of the main function in shader.usf


// Draw Screen Pass function copied directly from the engine OpenColorIODisplayExtension.cpp
// copy start
namespace {
	template<typename TSetupFunction>
	void DrawScreenPass(
		FRHICommandListImmediate& RHICmdList,
		const FSceneView& View,
		const FScreenPassTextureViewport& OutputViewport,
		const FScreenPassTextureViewport& InputViewport,
		const FScreenPassPipelineState& PipelineState,
		TSetupFunction SetupFunction)
	{
		PipelineState.Validate();

		const FIntRect InputRect = InputViewport.Rect;
		const FIntPoint InputSize = InputViewport.Extent;
		const FIntRect OutputRect = OutputViewport.Rect;
		const FIntPoint OutputSize = OutputRect.Size();

		RHICmdList.SetViewport(OutputRect.Min.X, OutputRect.Min.Y, 0.0f, OutputRect.Max.X, OutputRect.Max.Y, 1.0f);

		SetScreenPassPipelineState(RHICmdList, PipelineState);

		// Setting up buffers.
		SetupFunction(RHICmdList);

		FIntPoint LocalOutputPos(FIntPoint::ZeroValue);
		FIntPoint LocalOutputSize(OutputSize);
		EDrawRectangleFlags DrawRectangleFlags = EDRF_UseTriangleOptimization;

		DrawPostProcessPass(
			RHICmdList,
			LocalOutputPos.X, LocalOutputPos.Y, LocalOutputSize.X, LocalOutputSize.Y,
			InputRect.Min.X, InputRect.Min.Y, InputRect.Width(), InputRect.Height(),
			OutputSize,
			InputSize,
			PipelineState.VertexShader,
			View.StereoPass,
			false,
			DrawRectangleFlags);
	}

}
// copy end

FScreenPassTexture FSceneVEProcess::AddSceneVETestPass(FRDGBuilder& GraphBuilder, const FSceneView& SceneView, const FPostProcessMaterialInputs& Inputs)
{
	// SceneViewExtension gives SceneView, not ViewInfo so we need to setup some basics ourself
	const FSceneViewFamily& ViewFamily = *SceneView.Family;
	const ERHIFeatureLevel::Type FeatureLevel = SceneView.GetFeatureLevel();

	const FScreenPassTexture& SceneColor = Inputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor];

	if (!SceneColor.IsValid())
	{
		return SceneColor;
	}

	// Here starts the RDG stuff
	RDG_EVENT_SCOPE(GraphBuilder, "SceneVETestPass");
	{
		// Accesspoint to our Shaders
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ViewFamily.GetFeatureLevel());

		// Template Render Target to use as main output
		FScreenPassRenderTarget templateRenderTarget;

		// Check if this destination is the last one on the post process pipeline
		if (Inputs.OverrideOutput.IsValid())
		{
			templateRenderTarget = Inputs.OverrideOutput;
		}
		else
		// Otherwise make a template RenderTarget
		{
			FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
			OutputDesc.Flags |= TexCreate_RenderTargetable;
			FLinearColor ClearColor(0., 0., 0., 0.);
			OutputDesc.ClearValue = FClearValueBinding(ClearColor);

			FRDGTexture* templateRenderTargetTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("templateRenderTargetTexture"));
			templateRenderTarget = FScreenPassRenderTarget(templateRenderTargetTexture, SceneColor.ViewRect, ERenderTargetLoadAction::EClear);
		}

		// The Viewport in source and destination might be different
		const FScreenPassTextureViewport SceneColorViewport(SceneColor);
		const FScreenPassTextureViewport tempRenderTargetViewport(templateRenderTarget);

		FScreenPassRenderTarget SceneColorRenderTarget(SceneColor, ERenderTargetLoadAction::ELoad);

		// We need these for the GraphBuilder AddPass
		FRHIBlendState* DefaultBlendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI();
		FRHIDepthStencilState* DepthStencilState = FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

		{
			// Get the assigned shaders
			TShaderMapRef<FSceneVETestShaderVS> VertexShader(GlobalShaderMap);
			TShaderMapRef<FSceneVETestShaderPS> PixelShader(GlobalShaderMap);

			// Pass the shader parameters
			FSceneVETestShaderParameters* PassParameters = GraphBuilder.AllocParameters<FSceneVETestShaderParameters>();
			PassParameters->InputTexture = SceneColorRenderTarget.Texture;
			PassParameters->InputSampler = TStaticSamplerState<>::GetRHI();
			PassParameters->RenderTargets[0] = templateRenderTarget.GetRenderTargetBinding();

			// Add Pass by submitting the shaders and parameters to the GraphBuilder that takes care of submitting it to the Renderthread 
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("SceneVETestMainPass"),
				PassParameters,
				ERDGPassFlags::Raster,
				[&SceneView,
				VertexShader,
				PixelShader,
				DefaultBlendState,
				DepthStencilState,
				SceneColorViewport,
				tempRenderTargetViewport,
				PassParameters](FRHICommandListImmediate& RHICmdList)
			{
				DrawScreenPass(
					RHICmdList,
					SceneView,
					tempRenderTargetViewport,
					SceneColorViewport,
					FScreenPassPipelineState(VertexShader, PixelShader, DefaultBlendState, DepthStencilState),
					[&](FRHICommandListImmediate& RHICmdList)
				{
					SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);
				});
			});

	}

	// Return the result
	return MoveTemp(templateRenderTarget);
	}

}

