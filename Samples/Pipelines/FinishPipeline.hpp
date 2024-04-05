#pragma once

#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>

/*
Pipeline to present a swapchain image
*/
class FinishPipeline : public PipelineBase
{
public:
	FinishPipeline(GraphicsDevice* gd);

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
};