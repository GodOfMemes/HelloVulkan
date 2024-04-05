#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include <Graphics/Pipeline.hpp>

/*
    Clears a swapchain image
*/

class ClearPipeline : public PipelineBase
{
public:
    ClearPipeline(GraphicsDevice* gd);

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
};