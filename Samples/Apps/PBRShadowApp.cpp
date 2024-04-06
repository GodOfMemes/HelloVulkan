#include "PBRShadowApp.hpp"
#include "Graphics/Mesh/MeshScenePODs.hpp"
#include "imgui.h"

PBRShadowApp::PBRShadowApp()
    : SampleBase(AppConfig{ .Title = "PBR Shadow" })
{
    
}

void PBRShadowApp::OnLoad()
{
    SampleBase::OnLoad();

    uiData.shadowCasterPosition_ = { 2.5f , 20, 5 };
    camera->SetPositionAndTarget(glm::vec3(0,3,5), glm::vec3(0));

    shadowResource = graphicsDevice->AddResources<ShadowResource>(graphicsDevice);
    shadowResource->CreateSingleShadowMap();

    InitLights();

	// Initialize attachments
	InitSharedResource();

    // Image-Based Lighting
	iblResource = graphicsDevice->AddResources<IBLResource>(graphicsDevice, TEXTURE_DIR + "piazza_bologni_1k.hdr");

	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = MODEL_DIR + "Sponza/Sponza.gltf",
		},
		{
			.filename = MODEL_DIR + "Tachikoma/Tachikoma.gltf",
			.clickable = true
		},
		{
			.filename = MODEL_DIR + "Hexapod/Hexapod.gltf",
			.clickable = true
		}
	};
	bool supportDeviceAddress = true;
	scene = std::make_unique<MeshScene>(graphicsDevice, dataArray, supportDeviceAddress);

    // Model matrix for Tachikoma
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.15f, 0.35f, 1.5f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.7f, 0.7f, 0.7f));
	scene->UpdateModelMatrix( { .model = modelMatrix }, 1, 0);

	// Model matrix for Hexapod
	modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.62f, -1.5f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
	scene->UpdateModelMatrix( { .model = modelMatrix }, 2, 0);

    clear = graphicsDevice->AddPipeline<ClearPipeline>(graphicsDevice); // This is responsible to clear the swapchain image
    skybox = graphicsDevice->AddPipeline<SkyBoxPipeline>(
		graphicsDevice,
		&(iblResource->diffuseCubemap_),
		sharedResource,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassType::ColorClear | RenderPassType::DepthClear);
	shadowPtr_ = graphicsDevice->AddPipeline<ShadowPipeline>(graphicsDevice, scene.get(), shadowResource);
	// Opaque pass
	pbrOpaquePtr_ = graphicsDevice->AddPipeline<PBRShadowPipeline>(
		graphicsDevice,
		scene.get(),
		lightResource,
		iblResource,
		shadowResource,
		sharedResource,
		MaterialType::Opaque);
	// Transparent pass
	pbrTransparentPtr_ = graphicsDevice->AddPipeline<PBRShadowPipeline>(
		graphicsDevice,
		scene.get(),
		lightResource,
		iblResource,
		shadowResource,
		sharedResource,
		MaterialType::Transparent);
	lightPtr_ = graphicsDevice->AddPipeline<LightRenderPipeline>(graphicsDevice, lightResource, sharedResource);
    // Resolve multisampled color image to singlesampled color image
    resolve = graphicsDevice->AddPipeline<ResolveMSPipeline>(graphicsDevice,sharedResource);
    // This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	tonemap = graphicsDevice->AddPipeline<TonemapPipeline>(graphicsDevice, &(sharedResource->singleSampledColorImage_));
	imguiPtr_ = graphicsDevice->AddPipeline<ImGuiPipeline>(graphicsDevice, window, scene.get(), camera.get());
    finish = graphicsDevice->AddPipeline<FinishPipeline>(graphicsDevice);
}

void PBRShadowApp::OnUpdate(double dt)
{
    SampleBase::OnUpdate(dt);

    DrawImGui();
    CameraUBO ubo
    {
        .projection = camera->GetProjectionMatrix(),
        .view = camera->GetViewMatrix(),
        .position = glm::vec4(camera->GetPosition(),1)
    };
    skybox->SetCameraUBO(ubo);
	pbrOpaquePtr_->SetCameraUBO(ubo);
	pbrTransparentPtr_->SetCameraUBO(ubo);
	lightPtr_->SetCameraUBO(ubo);

    shadowPtr_->UpdateShadow(shadowResource, lightResource->lights_[0].position_);
    pbrOpaquePtr_->SetShadowMapConfigUBO(shadowResource->shadowUBO_);
    pbrTransparentPtr_->SetShadowMapConfigUBO(shadowResource->shadowUBO_);
}

void PBRShadowApp::OnRender(VkCommandBuffer commandBuffer)
{
    clear->FillCommandBuffer(commandBuffer);

    skybox->FillCommandBuffer(commandBuffer);
    shadowPtr_->FillCommandBuffer(commandBuffer);
    pbrOpaquePtr_->FillCommandBuffer(commandBuffer);
    pbrTransparentPtr_->FillCommandBuffer(commandBuffer);
    lightPtr_->FillCommandBuffer(commandBuffer);

    resolve->FillCommandBuffer(commandBuffer);
    tonemap->FillCommandBuffer(commandBuffer);

    imguiPtr_->FillCommandBuffer(commandBuffer);
    finish->FillCommandBuffer(commandBuffer);
}

void PBRShadowApp::OnShutdown()
{
    SampleBase::OnShutdown();
    scene.reset();
}

void PBRShadowApp::InitLights()
{
	// Lights (SSBO)
	lightResource = graphicsDevice->AddResources<LightResource>(graphicsDevice);
	lightResource->AddLights(
	{
		// The first light is used to generate the shadow map
		// and its position is set by ImGui
		{.color_ = glm::vec4(1.f), .radius_ = 1.0f },

		// Add additional lights so that the scene is not too dark
		{.position_ = glm::vec4(-1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void PBRShadowApp::DrawImGui()
{
    if (!uiData.imguiShow_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Shadow Mapping", 450, 700);
	imguiPtr_->ImGuiShowFrameData(&frameCounter);

	ImGui::Text("Triangle Count: %i", scene->triangleCount_);
	ImGui::Checkbox("Render Lights", &uiData.renderLights_);
	imguiPtr_->ImGuizmoShowOption(&uiData.gizmoMode_);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&uiData.pbrPC_, iblResource->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &uiData.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &uiData.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &uiData.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &uiData.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &uiData.shadowOrthoSize_, 10.0f, 30.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(uiData.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(uiData.shadowCasterPosition_[1]), 15.0f, 60.0f);
	ImGui::SliderFloat("Z", &(uiData.shadowCasterPosition_[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuizmoManipulateScene(&uiData);

	imguiPtr_->ImGuiEnd();

    {
        pbrOpaquePtr_->SetPBRPushConstants(uiData.pbrPC_);
        pbrTransparentPtr_->SetPBRPushConstants(uiData.pbrPC_);
        lightPtr_->shouldRender = uiData.renderLights_;
    }

    {
        shadowResource->shadowUBO_.shadowMinBias = uiData.shadowMinBias_;
		shadowResource->shadowUBO_.shadowMaxBias = uiData.shadowMaxBias_;
		shadowResource->shadowNearPlane_ = uiData.shadowNearPlane_;
		shadowResource->shadowFarPlane_ = uiData.shadowFarPlane_;
		shadowResource->orthoSize_ = uiData.shadowOrthoSize_;

        lightResource->UpdateLightPosition(0, uiData.shadowCasterPosition_);
    }
}
