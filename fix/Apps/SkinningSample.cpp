#include "SkinningSample.hpp"
#include "Graphics/RenderPass.hpp"
#include "imgui.h"

SkinningApp::SkinningApp()
    : SampleBase(AppConfig
    {
        .Title = "Skinning"
    })
{
    
}

void SkinningApp::OnLoad()
{
    uiData.shadowMinBias_ = 0.003;
    uiData.shadowCasterPosition_ = { -0.5, 20, 5};
    
    {
        camera->SetPosition({0,3,5});
        glm::vec3 dir = glm::normalize(glm::vec3(0) - glm::vec3(0,3,5));
        camera->SetPitch(std::clamp(glm::degrees(asin(dir.y)), -90.0,90.0));
        camera->SetYaw(glm::degrees(atan2(dir.z,dir.x)));
    }

    shadowResource = graphicsDevice->AddResources<ShadowResource>(graphicsDevice);
    shadowResource->CreateSingleShadowMap();

    lightResource = graphicsDevice->AddResources<LightResource>(graphicsDevice);
    lightResource->AddLights(
	{
		// The first light is a shadow caster
		{.color_ = glm::vec4(1.f), .radius_ = 1.0f },

		// Add additional lights so that the scene is not too dark
		{.position_ = glm::vec4(-1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});

    iblResource = graphicsDevice->AddResources<IBLResource>(graphicsDevice, TEXTURE_DIR + "piazza_bologni_1k.hdr");

    InitScene();

    SetupPipelines();
}

void SkinningApp::OnUpdate(double dt)
{
    DrawImGui();
	
    pbrOpaque->SetPBRPushConstants(uiData.pbrPC_);
    pbrTransparent->SetPBRPushConstants(uiData.pbrPC_);
    light->shouldRender = uiData.renderLights_;
    lightResource->UpdateFromUIData(uiData);
    shadowResource->UpdateFromUIData(uiData);

    //scene->UpdateAnimation(dt);
    shadow->UpdateShadow(shadowResource, lightResource->lights_[0].position_);
    pbrOpaque->SetShadowMapConfigUBO(shadowResource->shadowUBO_);
    pbrTransparent->SetShadowMapConfigUBO(shadowResource->shadowUBO_);
}

void SkinningApp::OnRender(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
    clear->FillCommandBuffer(commandBuffer);
   skybox->FillCommandBuffer(commandBuffer);
    skinning->FillCommandBuffer(commandBuffer);
    shadow->FillCommandBuffer(commandBuffer);
    pbrOpaque->FillCommandBuffer(commandBuffer);
    pbrTransparent->FillCommandBuffer(commandBuffer);
    light->FillCommandBuffer(commandBuffer);
    resolve->FillCommandBuffer(commandBuffer);
    tonemap->FillCommandBuffer(commandBuffer);
    imgui->FillCommandBuffer(commandBuffer);
    finish->FillCommandBuffer(commandBuffer);
}

void SkinningApp::OnShutdown()
{
    scene.reset();
}

void SkinningApp::InitScene()
{
    std::vector<ModelCreateInfo> dataArray = 
	{
    {
            .filename = MODEL_DIR + "Sponza/Sponza.gltf"
        },
    {
            .filename = MODEL_DIR + "DancingStormtrooper01/DancingStormtrooper01.gltf",
            .instanceCount = 4,
            .playAnimation = true,
            .clickable = true
        },
    {
            .filename = MODEL_DIR + "DancingStormtrooper02/DancingStormtrooper02.gltf",
            .instanceCount = 4,
            .playAnimation = true,
            .clickable = true
        }
    };
    scene = std::make_unique<MeshScene>(graphicsDevice,dataArray,true);

    constexpr uint32_t xCount = 2;
	constexpr uint32_t zCount = 4;
	constexpr float dist = 1.2f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	constexpr float yPos = -0.33f;
	const glm::vec3 scale = glm::vec3(0.5f, 0.5f, 0.5f);

	for (uint32_t x = 0; x < xCount; ++x)
	{
		for (uint32_t z = 0; z < zCount; ++z)
		{
			float xPos = static_cast<float>(x) * dist - xMidPos;
			float zPos = -(static_cast<float>(z) * dist) + zMidPos;
			glm::mat4 modelMatrix(1.f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(xPos, yPos, zPos));
			modelMatrix = glm::scale(modelMatrix, scale);
			uint32_t modelIndex = x % 2;
			uint32_t instanceIndex = z; // May not work if you change xCount and zCount
			scene->UpdateModelMatrix( { .model = modelMatrix }, modelIndex + 1, instanceIndex);
		}
	}
}

void SkinningApp::SetupPipelines()
{
    clear = graphicsDevice->AddPipeline<ClearPipeline>(graphicsDevice); // This is responsible to clear the swapchain image
    // This draws a cube
    skybox = graphicsDevice->AddPipeline<SkyBoxPipeline>(
        graphicsDevice,
        &iblResource->diffuseCubemap_,
        sharedResource,
        // This is the first offscreen render pass so we need to clear the color attachment and depth attachment
        RenderPassType::ColorClear | RenderPassType::DepthClear);
    skinning = graphicsDevice->AddPipeline<SkinningPipeline>(graphicsDevice,scene.get());
    shadow = graphicsDevice->AddPipeline<ShadowPipeline>(graphicsDevice,scene.get(), shadowResource);

    // OpaquePass
    pbrOpaque = graphicsDevice->AddPipeline<PBRShadowPipeline>(
        graphicsDevice,
        scene.get(),
        lightResource,
        iblResource,
        shadowResource,
        sharedResource,
        MaterialType::Opaque);
    // TransparentPass
    pbrTransparent = graphicsDevice->AddPipeline<PBRShadowPipeline>(
        graphicsDevice,
        scene.get(),
        lightResource,
        iblResource,
        shadowResource,
        sharedResource,
        MaterialType::Transparent);
    
    light = graphicsDevice->AddPipeline<LightRenderPipeline>(graphicsDevice,lightResource,sharedResource);
    // Resolve multisampled color image to singlesampled color image
    resolve = graphicsDevice->AddPipeline<ResolveMSPipeline>(graphicsDevice,sharedResource);
    // This is an on-screen render pass that transfers singlesampled color image to swapchain image
    tonemap = graphicsDevice->AddPipeline<TonemapPipeline>(graphicsDevice, &sharedResource->singleSampledColorImage_);

    imgui = graphicsDevice->AddPipeline<ImGuiPipeline>(graphicsDevice,window,scene.get(),camera.get());
    finish = graphicsDevice->AddPipeline<FinishPipeline>(graphicsDevice);
}

void SkinningApp::DrawImGui()
{
    if(uiData.imguiShow_)
    {
        imgui->ImGuiDrawEmpty();
        return;
    }

    imgui->ImGuiStart();
	imgui->ImGuiSetWindow("Compute-based Skinning", 450, 700);
	imgui->ImGuiShowFrameData(&frameCounter);

	ImGui::Text("Triangle Count: %i", scene->triangleCount_);
	ImGui::Checkbox("Render Lights", &uiData.renderLights_);
	imgui->ImGuizmoShowOption(&uiData.gizmoMode_);
	ImGui::SeparatorText("Shading");
	imgui->ImGuiShowPBRConfig(&uiData.pbrPC_, iblResource->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &uiData.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &uiData.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &uiData.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &uiData.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &uiData.shadowOrthoSize_, 10.0f, 100.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(uiData.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(uiData.shadowCasterPosition_[1]), 5.0f, 60.0f);
	ImGui::SliderFloat("Z", &(uiData.shadowCasterPosition_[2]), -10.0f, 10.0f);

	imgui->ImGuizmoManipulateScene(&uiData);
}
