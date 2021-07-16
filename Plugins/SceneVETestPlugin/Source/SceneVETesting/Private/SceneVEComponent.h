// Test project fo SceneViewExtension / RDG Shader Basic setup
// Copyright 2021 Ossi Luoto
// 
// SceneVEComponent - The SceneViewExtensionBase. Place this in the editor to an empty actor 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "RenderGraphUtils.h"

#include "SceneViewExtension.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess\PostProcessMaterial.h"

#include "SceneVEComponent.generated.h"

// Use FSceneViewExtensionBase to extend hook properly
class FTestSceneExtension : public FSceneViewExtensionBase
{
public:
	FTestSceneExtension(const FAutoRegister& AutoRegister);

	// These must all be set
public:
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {};
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override; // like component tick
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {};
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {};
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled);

	// This is our actual hook function
	FScreenPassTexture TestPostProcessPass_RT(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& InOutInputs);

private:

};

// SceneVEComponent. Simple spawnable component to be place in the editor to an empty or other actor

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SCENEVETESTING_API USceneVEComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USceneVEComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	void CreateSceneViewExtension();

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// this is the way to store the SceneExtensionView to keep it safe from not being destroyed - from: SceneViewExtension.h
	TSharedPtr<class FTestSceneExtension, ESPMode::ThreadSafe > TestSceneExtension;
};
