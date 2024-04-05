#include "AppSkinning.h"
#include "Utility.h"
#include "Configs.h"

#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineSkinning.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

#include <iostream>

AppSkinning::AppSkinning()
{
}

void AppSkinning::Init()
{

	InitScene();

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	// This draws a cube
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resourcesIBL_->diffuseCubemap_),
		resourcesShared_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	AddPipeline<PipelineSkinning>(vulkanContext_, scene_.get());
	shadowPtr_ = AddPipeline<PipelineShadow>(vulkanContext_, scene_.get(), resourcesShadow_);
	// Opaque pass
	pbrOpaquePtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShadow_,
		resourcesShared_,
		MaterialType::Opaque);
	// Transparent pass
	pbrTransparentPtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShadow_,
		resourcesShared_,
		MaterialType::Transparent);
	lightPtr_ = AddPipeline<PipelineLightRender>(vulkanContext_, resourcesLight_, resourcesShared_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resourcesShared_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resourcesShared_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_, scene_.get(), camera_.get());
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppSkinning::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	// Start
	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-based Skinning", 450, 700);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Triangle Count: %i", scene_->triangleCount_);
	ImGui::Checkbox("Render Lights", &uiData_.renderLights_);
	imguiPtr_->ImGuizmoShowOption(&uiData_.gizmoMode_);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&uiData_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &uiData_.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &uiData_.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &uiData_.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &uiData_.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &uiData_.shadowOrthoSize_, 10.0f, 100.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(uiData_.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(uiData_.shadowCasterPosition_[1]), 5.0f, 60.0f);
	ImGui::SliderFloat("Z", &(uiData_.shadowCasterPosition_[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuizmoManipulateScene(vulkanContext_, &uiData_);
	
	// End
	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromIUData(vulkanContext_, uiData_);
	}
	for (auto& resources : resources_)
	{
		resources->UpdateFromUIData(vulkanContext_, uiData_);
	}
}

void AppSkinning::UpdateUBOs()
{
	scene_->UpdateAnimation(vulkanContext_, frameCounter_.GetDeltaSecond());

	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}

	shadowPtr_->UpdateShadow(vulkanContext_, resourcesShadow_, resourcesLight_->lights_[0].position_);
	pbrOpaquePtr_->SetShadowMapConfigUBO(vulkanContext_, resourcesShadow_->shadowUBO_);
	pbrTransparentPtr_->SetShadowMapConfigUBO(vulkanContext_, resourcesShadow_->shadowUBO_);
}

// This is called from main.cpp
void AppSkinning::MainLoop()
{
	InitVulkan({
		.suportBufferDeviceAddress_ = true,
		.supportMSAA_ = true,
		.supportBindlessTextures_ = true,
	});
	Init();

	// Main loop
	while (StillRunning())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
		DrawFrame();
	}

	scene_.reset();

	DestroyResources();
}
