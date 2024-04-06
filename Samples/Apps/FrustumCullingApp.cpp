#include "FrustumCullingApp.hpp"

FrustumCullingApp::FrustumCullingApp()
    : SampleBase(AppConfig{ .Title = "Bindless"})
{
    
}

void FrustumCullingApp::OnLoad()
{
    uiData.pbrPC_.albedoMultipler = 0.5f;
	camera->SetPositionAndTarget(glm::vec3(0.0f, 10.0f, 5.0f), glm::vec3(0.0, 0.0, -20));

    InitLights();

    InitSharedResource();

    // Image-Based Lighting
	iblResource = graphicsDevice->AddResources<IBLResource>(graphicsDevice, TEXTURE_DIR + "piazza_bologni_1k.hdr");

	InitScene();

    clear = graphicsDevice->AddPipeline<ClearPipeline>(graphicsDevice); // This is responsible to clear the swapchain image
    skybox = graphicsDevice->AddPipeline<SkyBoxPipeline>(
		graphicsDevice,
		&(iblResource->diffuseCubemap_),
		sharedResource,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassType::ColorClear | RenderPassType::DepthClear);
	cullingPtr_ = graphicsDevice->AddPipeline<FrustumCullingPipeline>(graphicsDevice, scene.get());
	pbr = graphicsDevice->AddPipeline<PBRBindlessPipeline>(
		graphicsDevice,
		scene.get(),
		lightResource,
		iblResource,
		sharedResource,
		false);
	infGridPtr_ = graphicsDevice->AddPipeline<InfiniteGridPipeline>(graphicsDevice, sharedResource, 0.0f);
	boxRenderPtr_ = graphicsDevice->AddPipeline<AABBRenderPipeline>(graphicsDevice, sharedResource, scene.get());
	linePtr_ = graphicsDevice->AddPipeline<LinePipeline>(graphicsDevice, sharedResource, scene.get());
	lightPtr_ = graphicsDevice->AddPipeline<LightRenderPipeline>(graphicsDevice, lightResource, sharedResource);
    // Resolve multisampled color image to singlesampled color image
    resolve = graphicsDevice->AddPipeline<ResolveMSPipeline>(graphicsDevice,sharedResource);
    // This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	tonemap = graphicsDevice->AddPipeline<TonemapPipeline>(graphicsDevice, &(sharedResource->singleSampledColorImage_));
	imguiPtr_ = graphicsDevice->AddPipeline<ImGuiPipeline>(graphicsDevice, window, scene.get(), camera.get());
    finish = graphicsDevice->AddPipeline<FinishPipeline>(graphicsDevice);
}

void FrustumCullingApp::OnUpdate(double dt)
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
	infGridPtr_->SetCameraUBO(ubo);
	boxRenderPtr_->SetCameraUBO(ubo);
	linePtr_->SetCameraUBO(ubo);
	lightPtr_->SetCameraUBO(ubo);

	if(updateFrustum_)
	{
		FrustumUBO fubo(camera->GetProjectionMatrix(),camera->GetViewMatrix());
		linePtr_->SetFrustum(ubo);
		cullingPtr_->SetFrustumUBO(fubo);
	}
}

void FrustumCullingApp::OnRender(VkCommandBuffer commandBuffer)
{
    clear->FillCommandBuffer(commandBuffer);

    skybox->FillCommandBuffer(commandBuffer);
	cullingPtr_->FillCommandBuffer(commandBuffer);
    pbr->FillCommandBuffer(commandBuffer);
	infGridPtr_->FillCommandBuffer(commandBuffer);
	boxRenderPtr_->FillCommandBuffer(commandBuffer);
	linePtr_->FillCommandBuffer(commandBuffer);
    lightPtr_->FillCommandBuffer(commandBuffer);

    resolve->FillCommandBuffer(commandBuffer);
    tonemap->FillCommandBuffer(commandBuffer);

    imguiPtr_->FillCommandBuffer(commandBuffer);
    finish->FillCommandBuffer(commandBuffer);
}

void FrustumCullingApp::OnShutdown()
{
    scene.reset();
}

void FrustumCullingApp::InitLights()
{
    // Lights (SSBO)
	lightResource = graphicsDevice->AddResources<LightResource>(graphicsDevice);
	lightResource->AddLights(
	{
		{.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void FrustumCullingApp::InitScene()
{
	constexpr uint32_t xCount = 50;
	constexpr uint32_t zCount = 50;
	constexpr float dist = 4.0f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	std::vector<ModelCreateInfo> dataArray = {{ 
		.filename = MODEL_DIR + "Zaku/Zaku.gltf", 
		.instanceCount = xCount * zCount,
		.playAnimation = false
	}};
	bool supportDeviceAddress = true;
	scene = std::make_unique<MeshScene>(graphicsDevice, dataArray, supportDeviceAddress);
	uint32_t iter = 0;

	for (uint32_t x = 0; x < xCount; ++x)
	{
		for (uint32_t z = 0; z < zCount; ++z)
		{
			float xPos = static_cast<float>(x) * dist - xMidPos;
			float yPos = 0.0f;
			float zPos = -(static_cast<float>(z) * dist) + zMidPos;
			glm::mat4 modelMatrix(1.f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(xPos, yPos, zPos));
			scene->UpdateModelMatrix({ .model = modelMatrix }, 0, iter++);
		}
	}
}

void FrustumCullingApp::DrawImGui()
{
    if (!uiData.imguiShow_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
    }

	static bool staticUpdateFrustum = true;

    imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-Based Frustum Culling", 450, 375);
	imguiPtr_->ImGuiShowFrameData(&frameCounter);
	ImGui::Checkbox("Render Lights", &uiData.renderLights_);
	ImGui::Checkbox("Render Frustum and Bounding Boxes", &uiData.renderDebug_);
	ImGui::Checkbox("Update Frustum", &staticUpdateFrustum);
	imguiPtr_->ImGuiShowPBRConfig(&uiData.pbrPC_, iblResource->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	updateFrustum_ = staticUpdateFrustum;

    {
        pbr->SetPBRPushConstants(uiData.pbrPC_);
		infGridPtr_->shouldRender_ = uiData.renderInfiniteGrid_;
		boxRenderPtr_->shouldRender_ = uiData.renderDebug_;
		linePtr_->shouldRender_ = uiData.renderDebug_;
        lightPtr_->shouldRender = uiData.renderLights_;
    }

    {
        lightResource->UpdateLightPosition(0, uiData.shadowCasterPosition_);
    }
}
