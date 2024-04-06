#include "PBRBindlessApp.hpp"

PBRBindlessApp::PBRBindlessApp()
    : SampleBase(AppConfig{ .Title = "Bindless"})
{
    
}

void PBRBindlessApp::OnLoad()
{
    camera->SetPositionAndTarget(glm::vec3(0,3,5), glm::vec3(0));

    InitLights();

    InitSharedResource();

    // Image-Based Lighting
	iblResource = graphicsDevice->AddResources<IBLResource>(graphicsDevice, TEXTURE_DIR + "piazza_bologni_1k.hdr");

    std::vector<ModelCreateInfo> dataArray = 
    {
	{
			.filename = MODEL_DIR + "Sponza/Sponza.gltf",
		},
	{
			.filename = MODEL_DIR + "Tachikoma/Tachikoma.gltf",
			//.clickable = true
		}
	};
	bool supportDeviceAddress = true;
	scene = std::make_unique<MeshScene>(graphicsDevice, dataArray, supportDeviceAddress);

    // Model matrix for Tachikoma
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.62f, 0.f));
	scene->UpdateModelMatrix( { .model = modelMatrix }, 1, 0);

    clear = graphicsDevice->AddPipeline<ClearPipeline>(graphicsDevice); // This is responsible to clear the swapchain image
    skybox = graphicsDevice->AddPipeline<SkyBoxPipeline>(
		graphicsDevice,
		&(iblResource->diffuseCubemap_),
		sharedResource,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassType::ColorClear | RenderPassType::DepthClear);
	pbr = graphicsDevice->AddPipeline<PBRBindlessPipeline>(
		graphicsDevice,
		scene.get(),
		lightResource,
		iblResource,
		sharedResource,
		false);
	lightPtr_ = graphicsDevice->AddPipeline<LightRenderPipeline>(graphicsDevice, lightResource, sharedResource);
    // Resolve multisampled color image to singlesampled color image
    resolve = graphicsDevice->AddPipeline<ResolveMSPipeline>(graphicsDevice,sharedResource);
    // This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	tonemap = graphicsDevice->AddPipeline<TonemapPipeline>(graphicsDevice, &(sharedResource->singleSampledColorImage_));
	imguiPtr_ = graphicsDevice->AddPipeline<ImGuiPipeline>(graphicsDevice, window, scene.get(), camera.get());
    finish = graphicsDevice->AddPipeline<FinishPipeline>(graphicsDevice);
}

void PBRBindlessApp::OnUpdate(double dt)
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
	pbr->SetCameraUBO(ubo);
	lightPtr_->SetCameraUBO(ubo);
}

void PBRBindlessApp::OnRender(VkCommandBuffer commandBuffer)
{
    clear->FillCommandBuffer(commandBuffer);

    skybox->FillCommandBuffer(commandBuffer);
    pbr->FillCommandBuffer(commandBuffer);
    lightPtr_->FillCommandBuffer(commandBuffer);

    resolve->FillCommandBuffer(commandBuffer);
    tonemap->FillCommandBuffer(commandBuffer);

    imguiPtr_->FillCommandBuffer(commandBuffer);
    finish->FillCommandBuffer(commandBuffer);
}

void PBRBindlessApp::OnShutdown()
{
    scene.reset();
}

void PBRBindlessApp::InitLights()
{
    // Lights (SSBO)
	lightResource = graphicsDevice->AddResources<LightResource>(graphicsDevice);
	lightResource->AddLights(
	{
		{ .position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void PBRBindlessApp::DrawImGui()
{
    if (!uiData.imguiShow_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
    }

    imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Bindless Textures", 450, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter);
	ImGui::Text("Triangle Count: %i", scene->triangleCount_);
	ImGui::Checkbox("Render Lights", &uiData.renderLights_);
	imguiPtr_->ImGuiShowPBRConfig(&uiData.pbrPC_, iblResource->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

    {
        pbr->SetPBRPushConstants(uiData.pbrPC_);
        lightPtr_->shouldRender = uiData.renderLights_;
    }

    {
        lightResource->UpdateLightPosition(0, uiData.shadowCasterPosition_);
    }
}
