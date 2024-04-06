#pragma once

#include "Apps/SampleAppBase.hpp"
#include "Pipelines/Compute/AABBGeneratorPipeline.hpp"
#include "Pipelines/Compute/LightCullingPipeline.hpp"
#include "Pipelines/OffScreen/LightRenderPipeline.hpp"
#include "Pipelines/OffScreen/PBRClusterForwardPipeline.hpp"
#include "Pipelines/OffScreen/ResolveMSPipeline.hpp"
#include "Pipelines/OffScreen/SkyboxPipeline.hpp"
#include "Pipelines/OnScreen/ClearPipeline.hpp"
#include "Pipelines/OnScreen/FinishPipeline.hpp"
#include "Pipelines/OnScreen/ImGuiPipeline.hpp"
#include "Pipelines/OnScreen/TonemapPipeline.hpp"
#include "Resources/ClusterForwardResource.hpp"
#include "Resources/LightResource.hpp"
#include "Resources/IBLResource.hpp"

class PBRClusterForwardApp : public SampleBase
{
public:
    PBRClusterForwardApp();
protected:
    void OnLoad() override;
    void OnUpdate(double dt) override;
    void OnRender(VkCommandBuffer commandBuffer) override;
    void OnShutdown() override;

    // TODO: Move Clear and Finsh to graphicsDevice
    ClearPipeline* clear = nullptr;
    SkyBoxPipeline* skybox = nullptr;
    AABBGeneratorPipeline* aabbPtr_ = nullptr;
	LightCullingPipeline* lightCullPtr_ = nullptr;
    PBRClusterForwardPipeline* pbrOpaquePtr_ = nullptr;
	PBRClusterForwardPipeline* pbrTransparentPtr_ = nullptr;
	LightRenderPipeline* lightPtr_ = nullptr;
    ResolveMSPipeline* resolve = nullptr;
    TonemapPipeline* tonemap = nullptr;
    ImGuiPipeline* imguiPtr_ = nullptr;
    FinishPipeline* finish = nullptr;

    ClusterForwardResource* cfResource;
    LightResource* lightResource;
    IBLResource* iblResource;

    std::unique_ptr<MeshScene> scene;

    void InitLights();
    void DrawImGui();
};