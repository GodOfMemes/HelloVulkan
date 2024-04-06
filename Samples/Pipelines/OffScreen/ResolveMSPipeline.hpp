#pragma once

#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>
#include "Resources/SharedResource.hpp"

/*
This pipeline does not draw anything but it resolves 
a multi-sampled color image to a single-sampled color image
*/
class ResolveMSPipeline : public PipelineBase
{
public:
	ResolveMSPipeline(
		GraphicsDevice* gd,
		SharedResource* sharedRes
	);
	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
};