#pragma once

#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>
#include "Graphics/Texture2D.hpp"
#include "Resources/SharedResource.hpp"

class SkyBoxPipeline : public PipelineBase
{
public:
    SkyBoxPipeline(
        GraphicsDevice* gd,
        Texture2D* envMap,
        SharedResource* sharedResource, 
        uint8_t renderType = 0);

    void SetCameraUBO(CameraUBO& ubo) override;
    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
private:
    Texture2D* envCubeMap;
    std::array<VkDescriptorSet,MAX_FRAMES_IN_FLIGHT> descriptorSets;

    void CreateDescriptor();
};