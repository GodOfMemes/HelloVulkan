#pragma once

#include "Graphics/Mesh/MeshScene.hpp"
#include "Pipelines/ClearPipeline.hpp"
#include "Pipelines/FinishPipeline.hpp"
#include "Pipelines/ImGuiPipeline.hpp"
#include "Pipelines/LightRenderPipeline.hpp"
#include "Pipelines/PBRShadowPipeline.hpp"
#include "Pipelines/ResolveMSPipeline.hpp"
#include "Pipelines/ShadowPipeline.hpp"
#include "Pipelines/SkyboxPipeline.hpp"
#include "Pipelines/TonemapPipeline.hpp"
#include "Resources/IBLResource.hpp"
#include "Resources/LightResource.hpp"
#include "Resources/ShadowResource.hpp"
#include "SampleAppBase.hpp"

class PBRShadowApp : public SampleBase
{
public:
    PBRShadowApp();
protected:
    void OnLoad() override;
    void OnUpdate(double dt) override;
    void OnRender(VkCommandBuffer commandBuffer) override;
    void OnShutdown() override;

    // TODO: Move Clear and Finsh to graphicsDevice
    ClearPipeline* clear = nullptr;
    SkyBoxPipeline* skybox = nullptr;
    ShadowPipeline* shadowPtr_ = nullptr;
    PBRShadowPipeline* pbrOpaquePtr_ = nullptr;
	PBRShadowPipeline* pbrTransparentPtr_ = nullptr;
	LightRenderPipeline* lightPtr_ = nullptr;
    ResolveMSPipeline* resolve = nullptr;
    TonemapPipeline* tonemap = nullptr;
    ImGuiPipeline* imguiPtr_ = nullptr;
    FinishPipeline* finish;

    ShadowResource* shadowResource;
    LightResource* lightResource;
    IBLResource* iblResource;

    std::unique_ptr<MeshScene> scene;

    void InitLights();
    void DrawImGui();
};