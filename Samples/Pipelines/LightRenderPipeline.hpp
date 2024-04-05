#pragma once

#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>
#include "Resources/LightResource.hpp"
#include "Resources/SharedResource.hpp"

/*
Render a light source for debugging purpose
*/
class LightRenderPipeline : public PipelineBase
{
public:
    LightRenderPipeline(
        GraphicsDevice* gd, 
        LightResource* lightRes, 
        SharedResource* sharedRes, 
        uint8_t renderType = 0);

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

    bool shouldRender;
private:
    LightResource* lightResource;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    void CreateDescriptor();
};