#include "PBRClusterForwardApp.hpp"
#include "Graphics/GraphicsStructs.hpp"
#include "Graphics/Mesh/MeshScenePODs.hpp"
#include "Utility/Utility.hpp"
#include "imgui.h"

PBRClusterForwardApp::PBRClusterForwardApp()
    : SampleBase(AppConfig{ .Title = "PBR ClusterForward" })
{
    
}

void PBRClusterForwardApp::OnLoad()
{
    SampleBase::OnLoad();

    uiData.pbrPC_.albedoMultipler = 0.01f;
    camera->SetPositionAndTarget(glm::vec3(0,1,6), glm::vec3(0,2.5,0));

    InitLights();

	// Initialize attachments
	InitSharedResource();

    // Image-Based Lighting
	iblResource = graphicsDevice->AddResources<IBLResource>(graphicsDevice, TEXTURE_DIR + "dikhololo_night_4k.hdr");

    cfResource = graphicsDevice->AddResources<ClusterForwardResource>(graphicsDevice);
	cfResource->CreateBuffers(lightResource->GetLightCount());

	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = MODEL_DIR + "Sponza/Sponza.gltf",
			.instanceCount = 1,
			.playAnimation = false
		}
	};
	bool supportDeviceAddress = true;
	scene = std::make_unique<MeshScene>(graphicsDevice, dataArray, supportDeviceAddress);

    clear = graphicsDevice->AddPipeline<ClearPipeline>(graphicsDevice); // This is responsible to clear the swapchain image
    skybox = graphicsDevice->AddPipeline<SkyBoxPipeline>(
		graphicsDevice,
		&(iblResource->environmentCubemap_),
		sharedResource,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassType::ColorClear | RenderPassType::DepthClear);
	aabbPtr_ = graphicsDevice->AddPipeline<AABBGeneratorPipeline>(graphicsDevice, cfResource);
	lightCullPtr_ = graphicsDevice->AddPipeline<LightCullingPipeline>(graphicsDevice, lightResource, cfResource);
	// Opaque pass
	pbrOpaquePtr_ = graphicsDevice->AddPipeline<PBRClusterForwardPipeline>(
		graphicsDevice,
		scene.get(),
		lightResource,
        cfResource,
		iblResource,
		sharedResource,
		MaterialType::Opaque);
	// Transparent pass
	pbrTransparentPtr_ = graphicsDevice->AddPipeline<PBRClusterForwardPipeline>(
		graphicsDevice,
		scene.get(),
        lightResource,
        cfResource,
		iblResource,
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

void PBRClusterForwardApp::OnUpdate(double dt)
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

    ClusterForwardUBO cfUBO
    (
        camera->GetInvProjectionMatrix(),
        camera->GetViewMatrix(),
        window->GetSize(),
        camera->GetNearPlane(),
        camera->GetFarPlane()
    );

    aabbPtr_->SetClusterForwardUBO(cfUBO);
	lightCullPtr_->ResetGlobalIndex();
	lightCullPtr_->SetClusterForwardUBO(cfUBO);
	pbrOpaquePtr_->SetClusterForwardUBO(cfUBO);
	pbrTransparentPtr_->SetClusterForwardUBO(cfUBO);
}

void PBRClusterForwardApp::OnRender(VkCommandBuffer commandBuffer)
{
    clear->FillCommandBuffer(commandBuffer);

    skybox->FillCommandBuffer(commandBuffer);
    aabbPtr_->FillCommandBuffer(commandBuffer);
    lightCullPtr_->FillCommandBuffer(commandBuffer);
    pbrOpaquePtr_->FillCommandBuffer(commandBuffer);
    pbrTransparentPtr_->FillCommandBuffer(commandBuffer);
    lightPtr_->FillCommandBuffer(commandBuffer);

    resolve->FillCommandBuffer(commandBuffer);
    tonemap->FillCommandBuffer(commandBuffer);

    imguiPtr_->FillCommandBuffer(commandBuffer);
    finish->FillCommandBuffer(commandBuffer);
}

void PBRClusterForwardApp::OnShutdown()
{
    SampleBase::OnShutdown();
    scene.reset();
}

void PBRClusterForwardApp::InitLights()
{
	std::vector<LightData> lights;

	float pi2 = glm::two_pi<float>();
	constexpr uint32_t NR_LIGHTS = 1000;
	for (uint32_t i = 0; i < NR_LIGHTS; ++i)
	{
		float yPos = Utility::RandomNumber(-2.f, 10.0f);
		float radius = Utility::RandomNumber(0.0f, 10.0f);
		float rad = Utility::RandomNumber(0.0f, pi2);
		float xPos = glm::cos(rad);

		glm::vec4 position(
			glm::cos(rad) * radius,
			yPos,
			glm::sin(rad) * radius,
			1.f
		);

		glm::vec4 color(
			Utility::RandomNumber(0.0f, 1.0f),
			Utility::RandomNumber(0.0f, 1.0f),
			Utility::RandomNumber(0.0f, 1.0f),
			1.f
		);

		LightData l;
		l.color_ = color;
		l.position_ = position;
		l.radius_ = Utility::RandomNumber(0.5f, 2.0f);

		lights.push_back(l);
	}

	lightResource = graphicsDevice->AddResources<LightResource>(graphicsDevice);
	lightResource->AddLights(lights);
}

void PBRClusterForwardApp::DrawImGui()
{
    if (!uiData.imguiShow_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Clustered Forward Shading", 450, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter);
	ImGui::Checkbox("Render Lights", &uiData.renderLights_);
	imguiPtr_->ImGuiShowPBRConfig(&uiData.pbrPC_, iblResource->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

    {
        pbrOpaquePtr_->SetPBRPushConstants(uiData.pbrPC_);
        pbrTransparentPtr_->SetPBRPushConstants(uiData.pbrPC_);
        lightPtr_->shouldRender = uiData.renderLights_;
    }

    {
        lightResource->UpdateLightPosition(0, uiData.shadowCasterPosition_);
    }
}