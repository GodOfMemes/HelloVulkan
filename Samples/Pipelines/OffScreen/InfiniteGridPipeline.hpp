#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Pipeline.hpp"
#include "Resources/SharedResource.hpp"

class InfiniteGridPipeline : public PipelineBase
{
public:
	InfiniteGridPipeline(
		GraphicsDevice* ctx,
		SharedResource* resourcesShared,
		float yPosition,
		uint8_t renderBit = 0);
	~InfiniteGridPipeline();

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

    bool shouldRender_;
private:
	float yPosition_;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

    void CreateDescriptor();
};