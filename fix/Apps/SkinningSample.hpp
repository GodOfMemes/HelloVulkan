#pragma once

#include "SampleAppBase.hpp"
#include <Graphics/Mesh/MeshScene.hpp>

#include "Pipelines/ClearPipeline.hpp"
#include "Pipelines/SkyboxPipeline.hpp"
#include "Pipelines/SkinningPipeline.hpp"
#include "Pipelines/ShadowPipeline.hpp"
#include "Pipelines/PBRShadowPipeline.hpp"
#include "Pipelines/LightRenderPipeline.hpp"
#include "Pipelines/ResolveMSPipeline.hpp"
#include "Pipelines/TonemapPipeline.hpp"
#include "Pipelines/ImGuiPipeline.hpp"
#include "Pipelines/FinishPipeline.hpp"

#include "Resources/LightResource.hpp"
#include "Resources/ShadowResource.hpp"

class SkinningApp : public SampleBase
{
public:
    SkinningApp();
protected:
   void OnLoad() override;
   void OnUpdate(double dt) override;
   void OnRender(VkCommandBuffer commandBuffer, uint32_t currentFrame) override;
   void OnShutdown() override;

    void InitScene();
    void SetupPipelines();
    void DrawImGui();

    // TODO: Move Clear and Finsh to graphicsDevice
    ClearPipeline* clear;
    SkyBoxPipeline* skybox;
    SkinningPipeline* skinning;
    ShadowPipeline* shadow;
    PBRShadowPipeline* pbrOpaque;
    PBRShadowPipeline* pbrTransparent;
    LightRenderPipeline* light;
    ResolveMSPipeline* resolve;
    TonemapPipeline* tonemap;
    ImGuiPipeline* imgui;
    FinishPipeline* finish;

    ShadowResource* shadowResource;
    LightResource* lightResource;

    std::unique_ptr<MeshScene> scene;
}; 