#pragma once

#include "Graphics/Mesh/MeshScene.hpp"
#include "Pipelines/OffScreen/AABBRenderPipeline.hpp"
#include "Pipelines/OnScreen/ClearPipeline.hpp"
#include "Pipelines/OnScreen/FinishPipeline.hpp"
#include "Pipelines/Compute/FrustumCullingPipeline.hpp"
#include "Pipelines/OnScreen/ImGuiPipeline.hpp"
#include "Pipelines/OffScreen/InfiniteGridPipeline.hpp"
#include "Pipelines/OffScreen/LinePipeline.hpp"
#include "Pipelines/OffScreen/PBRBindlessPipeline.hpp"
#include "Pipelines/OffScreen/LightRenderPipeline.hpp"
#include "Pipelines/OffScreen/ResolveMSPipeline.hpp"
#include "Pipelines/OffScreen/SkyboxPipeline.hpp"
#include "Pipelines/OnScreen/TonemapPipeline.hpp"
#include "Resources/IBLResource.hpp"
#include "Resources/LightResource.hpp"
#include "SampleAppBase.hpp"

class FrustumCullingApp : public SampleBase
{
public:
    FrustumCullingApp();
protected:
    void OnLoad() override;
    void OnUpdate(double dt) override;
    void OnRender(VkCommandBuffer commandBuffer) override;
    void OnShutdown() override;
private:
    ClearPipeline* clear = nullptr;
    SkyBoxPipeline* skybox = nullptr;
    FrustumCullingPipeline* cullingPtr_ = nullptr;
    PBRBindlessPipeline* pbr = nullptr;
    InfiniteGridPipeline* infGridPtr_ = nullptr;
	AABBRenderPipeline* boxRenderPtr_ = nullptr;
    LinePipeline* linePtr_ = nullptr;
	LightRenderPipeline* lightPtr_ = nullptr;
    ResolveMSPipeline* resolve = nullptr;
    TonemapPipeline* tonemap = nullptr;
    ImGuiPipeline* imguiPtr_ = nullptr;
    FinishPipeline* finish = nullptr;

    std::unique_ptr<MeshScene> scene = nullptr;
    LightResource* lightResource = nullptr;
    IBLResource* iblResource = nullptr;

    bool updateFrustum_ = true;
    
    void InitLights();
    void InitScene();
    void DrawImGui();
};