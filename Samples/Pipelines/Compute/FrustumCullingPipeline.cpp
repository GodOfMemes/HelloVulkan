#include "FrustumCullingPipeline.hpp"
#include "Graphics/Barrier.hpp"
#include "Graphics/GraphicsDevice.hpp"

FrustumCullingPipeline::FrustumCullingPipeline(GraphicsDevice* ctx, MeshScene* scene) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	}),
	scene_(scene)
{
	Buffer::CreateMultipleUniformBuffers(ctx, frustumBuffers_, sizeof(FrustumUBO), MAX_FRAMES_IN_FLIGHT);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);
	CreateComputePipeline(SHADER_DIR + "FrustumCulling.comp");
}

FrustumCullingPipeline::~FrustumCullingPipeline()
{
	for (auto buffer : frustumBuffers_)
	{
		buffer.Destroy();
	}
}

void FrustumCullingPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	Execute(commandBuffer, gd->GetCurrentFrameIdx());
}

void FrustumCullingPipeline::Execute(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	TracyVkZoneC(gd->GetCurrentFrame().TracyContext, commandBuffer, "Frustum_Culling", tracy::Color::ForestGreen);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSets_[frameIndex],
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	gd->InsertDebugLabel(commandBuffer, "FrustumCullingPipeline", 0xff9999ff);

	vkCmdDispatch(commandBuffer, scene_->GetInstanceCount(), 1, 1);

	const VkBufferMemoryBarrier2 bufferBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR,
		.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT_KHR ,
		.srcQueueFamilyIndex = gd->GetGraphicsFamily(),
		.dstQueueFamilyIndex = gd->GetGraphicsFamily(),
		.buffer = scene_->indirectBuffer_.GetHandle(),
		.offset = 0,
		.size = scene_->indirectBuffer_.GetSize(),
	};
	Barrier::CreateBufferBarrier(commandBuffer, &bufferBarrier, 1u);
}

void FrustumCullingPipeline::CreateDescriptor()
{
	constexpr uint32_t frameCount = MAX_FRAMES_IN_FLIGHT;
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlag); // 0
	dsInfo.AddBuffer(&(scene_->transformedBoundingBoxBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	dsInfo.AddBuffer(&(scene_->indirectBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	descriptor.CreatePoolAndLayout( dsInfo, frameCount, 1u);
	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(frustumBuffers_[i]), 0);
		descriptor.CreateSet( dsInfo, &(descriptorSets_[i]));
	}
}