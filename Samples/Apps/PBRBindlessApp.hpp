#pragma once

#include "Graphics/Mesh/MeshScene.hpp"
#include "Pipelines/OffScreen/LightRenderPipeline.hpp"
#include "Pipelines/OffScreen/PBRBindlessPipeline.hpp"
#include "Pipelines/OffScreen/ResolveMSPipeline.hpp"
#include "Pipelines/OffScreen/SkyboxPipeline.hpp"
#include "Pipelines/OnScreen/ClearPipeline.hpp"
#include "Pipelines/OnScreen/FinishPipeline.hpp"
#include "Pipelines/OnScreen/ImGuiPipeline.hpp"
#include "Pipelines/OnScreen/TonemapPipeline.hpp"
#include "SampleAppBase.hpp"

class PBRBindlessApp : public SampleBase
{
public:
    PBRBindlessApp();
protected:
    void OnLoad() override;
    void OnUpdate(double dt) override;
    void OnRender(VkCommandBuffer commandBuffer) override;
    void OnShutdown() override;
private:
    ClearPipeline* clear = nullptr;
    SkyBoxPipeline* skybox = nullptr;
    PBRBindlessPipeline* pbr = nullptr;
	LightRenderPipeline* lightPtr_ = nullptr;
    ResolveMSPipeline* resolve = nullptr;
    TonemapPipeline* tonemap = nullptr;
    ImGuiPipeline* imguiPtr_ = nullptr;
    FinishPipeline* finish = nullptr;

    std::unique_ptr<MeshScene> scene = nullptr;
    LightResource* lightResource = nullptr;
    IBLResource* iblResource = nullptr;

    void InitLights();
    void DrawImGui();
};