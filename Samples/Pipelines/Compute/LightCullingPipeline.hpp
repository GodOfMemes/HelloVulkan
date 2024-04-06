#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Pipeline.hpp"
#include "Resources/ClusterForwardResource.hpp"
#include "Resources/LightResource.hpp"
/*
Clustered Forward
*/
class LightCullingPipeline : public PipelineBase
{
public:
	LightCullingPipeline(GraphicsDevice* ctx, LightResource* resourcesLight, ClusterForwardResource* resourcesCF);
	~LightCullingPipeline();

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
	void ResetGlobalIndex();
	void SetClusterForwardUBO(ClusterForwardUBO& ubo);

private:
	LightResource* resourcesLight_;
	ClusterForwardResource* resourcesCF_;

	std::vector<Buffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

private:
	void Execute(VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor();
};