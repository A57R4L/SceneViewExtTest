// Test project fo SceneViewExtension / RDG Shader Basic setup
// Copyright 2021 - 2022 Ossi Luoto
// 
// SceneVEProcess.cpp - Actual RDG hook and Shader Loading

#include "SceneVEProcess.h"

// Shader implementation Macro doesn't work on .h file so load them here
IMPLEMENT_GLOBAL_SHADER(FSceneVETestShaderPS, "/Plugins/SceneVETestPlugin/SceneVETestShaderPS.usf", "MainPS", SF_Pixel);    // point to the shader  file, name of the main function in shader.usf
IMPLEMENT_GLOBAL_SHADER(FSceneVETestShaderVS, "/Plugins/SceneVETestPlugin/SceneVETestShaderVS.usf", "MainVS", SF_Vertex);    // point to the shader  file, name of the main function in shader.usf
IMPLEMENT_GLOBAL_SHADER(FSceneVETestShaderCS, "/Plugins/SceneVETestPlugin/SceneVETestShaderCS.usf", "MainCS", SF_Compute);    // point to the shader  file, name of the main function in shader.usf


namespace
{
	TAutoConsoleVariable<int32> CVarShaderSelector(
		TEXT("r.SceneVETest.Shader"),
		0,
		TEXT("Select shader to use in the post processing \n")
		TEXT(" 0: Vertex & Pixel Shader (default);")
		TEXT(" 1: Computer Shader."),
		ECVF_RenderThreadSafe);
}

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
#if ENGINE_MAJOR_VERSION == 5
			View.StereoViewIndex,
#else
			View.StereoPass,
#endif
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

	// Check which shader method to use VS/PS or CS

	// If you are f.ex. combining VS&PS and CS shaders and doing multiple passes, a lot of the setup code below works for both
	// Here both are declared independently just to make it easier to strip either one out if aiming for a simple setup
	// And also to point out a few different ways to do the setup

	if (CVarShaderSelector.GetValueOnRenderThread() == 0)
	{
		// Vertex/Pixel Shader version

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

				// Add Pass by submitting the shaders and parameters to the GraphBuilder that takes care of scheduling it to the Renderthread 
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

	else

	{
		// Compute Shader version
		
		// Here starts the RDG stuff
		RDG_EVENT_SCOPE(GraphBuilder, "SceneVETestPass");
		{
			// Accesspoint to our Shaders
			FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ViewFamily.GetFeatureLevel());

			// Setup all the desciptors.
			FRDGTextureDesc OutputDesc;
			{
				OutputDesc = SceneColor.Texture->Desc;

				OutputDesc.Reset();
				OutputDesc.Flags |= TexCreate_UAV;
				OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);

				FLinearColor ClearColor(0., 0., 0., 0.);
				OutputDesc.ClearValue = FClearValueBinding(ClearColor);
				
			}

			FRDGTextureRef templateRenderTargetTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("templateRenderTargetTexture"));
			
			// Set the shader parameters
			FSceneVETestShaderCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FSceneVETestShaderCS::FParameters>();

			// Input is the SceneColor from PostProcess Material Inputs
			PassParameters->OriginalSceneColor = SceneColor.Texture;

			// There are other ways to obtain this information, but for a reference this is a hack to make a FViewInfo from the SceneView that can be very useful
			checkSlow(SceneView->bIsViewInfo);
			const FViewInfo* View = (FViewInfo*)&SceneView;

			// Important! There are many places where UE sets and scales viewport resolution = what kind of scaling you might need to apply to get
			// your UV's right might depend where you place your custom post processing
			// With this example, we are doing the post processing AFTER TAA, which has already scaled up (if needed) the viewport
			// However, at least in UE 5.0 the UniformBuffer is populated with the initial View and Scale, meaning UV's and viewport Size there will be wrong
			// ie. below might work pre post processing chain, but not at this point
			//			FIntPoint PassViewSize = View->ViewRect.Size();
			//			PassParameters->ViewportRect = View->ViewRect;
			// You could also try to use: 			const float ScreenPercentage = ViewFamily.GetPrimaryResolutionFractionUpperBound() * ViewFamily.SecondaryViewFraction;
			// To obtain a scaling factor for UniformViewBuffer UV's, but at least for 5.0.1 this doesn't seem to honor the changing of project settings manual scaling
			// If this is a bug or a wrong approach to scaling, we'll have to see

			// So we use this instead

			// Get the input size (do note that this is not the full extent but the part of the texture containing the SceneColor information)
			FIntPoint PassViewSize = SceneView.UnconstrainedViewRect.Size();
			PassParameters->ViewportRect = SceneView.UnconstrainedViewRect;

			// As the previously mentioned parameters doesn't seem to honor the changes fully, let's just calculate BufferInv from the texture we just got
#if ENGINE_MAJOR_VERSION == 5
			PassParameters->SceneColorBufferInvSize = FVector2f(1.0f / SceneColor.Texture->Desc.Extent.X, 1.0f / SceneColor.Texture->Desc.Extent.Y);
#else
			PassParameters->SceneColorBufferInvSize = FVector2D(1.0f / SceneColor.Texture->Desc.Extent.X, 1.0f / SceneColor.Texture->Desc.Extent.Y);
#endif

			// Method to setup common parameters, we use this to pass ViewUniformBuffer data
			// There is a lot of useful stuff in the ViewUniformBuffer, do note that when getting this from the SceneView, a lot them seem to be unpopulated
			FCommonShaderParameters CommonParameters;
			CommonParameters.ViewUniformBuffer = SceneView.ViewUniformBuffer;
			PassParameters->CommonParameters = CommonParameters;

			// Create UAV from Target Texture
			PassParameters->Output = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(templateRenderTargetTexture));

			// Set groupcount and execute pass
			const int32 kDefaultGroupSize = 8;
			FIntPoint GroupSize(kDefaultGroupSize, kDefaultGroupSize);
			FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(PassViewSize, GroupSize);

			TShaderMapRef<FSceneVETestShaderCS> ComputeShader(GlobalShaderMap);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("SceneVETest CS Shader %dx%d", PassViewSize.X, PassViewSize.Y),
				ComputeShader,
				PassParameters,
				GroupCount);

			// One can use a similar method as in the VS/PS version to create a ScreenPassRenderTarget and return with the MoveTemp
			// We are doing here another trick to do a fast copy from the template buffer to the original scenecolor
			// If you are doing multiple passes, technically you should be able to even use the original scenecolor RT as the final output buffer as UAV
			// However, for some reason that seems to work very randomy, so usually with multiple passes, this is the last method I use with multiple Compute Shaders

			AddCopyTexturePass(GraphBuilder, templateRenderTargetTexture, SceneColor.Texture);

			return SceneColor;
		}


	}

}

