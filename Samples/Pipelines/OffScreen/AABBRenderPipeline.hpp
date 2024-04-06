#pragma once

//AABBRenderPipeline

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Mesh/MeshScene.hpp"
#include "Graphics/Pipeline.hpp"
#include "Resources/SharedResource.hpp"

class AABBRenderPipeline : public PipelineBase
{
public:
	AABBRenderPipeline(
		GraphicsDevice* ctx,
		SharedResource* resShared,
		MeshScene* scene,
		uint8_t renderBit = 0u
	);
	~AABBRenderPipeline();

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

    bool shouldRender_;

private:
	MeshScene* scene_;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

    void CreateDescriptor();
	
};