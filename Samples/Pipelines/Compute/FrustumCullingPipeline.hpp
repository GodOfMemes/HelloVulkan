#pragma once


#include "Graphics/Buffer.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Mesh/MeshScene.hpp"
#include "Graphics/Pipeline.hpp"

class FrustumCullingPipeline : public PipelineBase
{
public:
	FrustumCullingPipeline(GraphicsDevice* ctx, MeshScene* scene);
	~FrustumCullingPipeline();

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

	void SetFrustumUBO(FrustumUBO& ubo)
	{
		frustumBuffers_[gd->GetCurrentFrameIdx()].UploadBufferData( &ubo, sizeof(FrustumUBO));
	}

private:
	void Execute(VkCommandBuffer commandBuffer, uint32_t frameIndex);

	void CreateDescriptor();

private:
	MeshScene* scene_;
	std::vector<Buffer> frustumBuffers_;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;
};