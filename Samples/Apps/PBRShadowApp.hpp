#pragma once

#include "Graphics/Mesh/MeshScene.hpp"
#include "Pipelines/OffScreen/LightRenderPipeline.hpp"
#include "Pipelines/OffScreen/PBRShadowPipeline.hpp"
#include "Pipelines/OffScreen/ResolveMSPipeline.hpp"
#include "Pipelines/OffScreen/ShadowPipeline.hpp"
#include "Pipelines/OffScreen/SkyboxPipeline.hpp"
#include "Pipelines/OnScreen/ClearPipeline.hpp"
#include "Pipelines/OnScreen/FinishPipeline.hpp"
#include "Pipelines/OnScreen/ImGuiPipeline.hpp"
#include "Pipelines/OnScreen/TonemapPipeline.hpp"
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
    FinishPipeline* finish = nullptr;

    ShadowResource* shadowResource;
    LightResource* lightResource;
    IBLResource* iblResource;

    std::unique_ptr<MeshScene> scene;

    void InitLights();
    void DrawImGui();
};