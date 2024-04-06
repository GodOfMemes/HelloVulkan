#pragma once

#include "Pipelines/Compute/SkinningPipeline.hpp"
#include "Pipelines/OffScreen/LightRenderPipeline.hpp"
#include "Pipelines/OffScreen/PBRShadowPipeline.hpp"
#include "Pipelines/OffScreen/ResolveMSPipeline.hpp"
#include "Pipelines/OffScreen/ShadowPipeline.hpp"
#include "Pipelines/OffScreen/SkyboxPipeline.hpp"
#include "Pipelines/OnScreen/ClearPipeline.hpp"
#include "Pipelines/OnScreen/FinishPipeline.hpp"
#include "Pipelines/OnScreen/ImGuiPipeline.hpp"
#include "Pipelines/OnScreen/TonemapPipeline.hpp"
#include "SampleAppBase.hpp"
#include <Graphics/Mesh/MeshScene.hpp>
#include "Resources/LightResource.hpp"
#include "Resources/ShadowResource.hpp"

class SkinningApp : public SampleBase
{
public:
    SkinningApp();
protected:
   void OnLoad() override;
   void OnUpdate(double dt) override;
   void OnRender(VkCommandBuffer commandBuffer) override;
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
    IBLResource* iblResource;

    std::unique_ptr<MeshScene> scene;
}; 