#include "AABBGeneratorPipeline.hpp"
#include <Graphics/Barrier.hpp>

AABBGeneratorPipeline::AABBGeneratorPipeline(
	GraphicsDevice* ctx, 
	ClusterForwardResource* resourcesCF) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	}),
	cfResource(resourcesCF)
{
    Buffer::CreateMultipleUniformBuffers(gd, cfUboBuffers, sizeof(ClusterForwardUBO), MAX_FRAMES_IN_FLIGHT);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);
	CreateComputePipeline(SHADER_DIR + "ClusteredForward/AABBGenerator.comp");
}

AABBGeneratorPipeline::~AABBGeneratorPipeline()
{
	for (auto uboBuffer : cfUboBuffers)
	{
		uboBuffer.Destroy();
	}
}

void AABBGeneratorPipeline::OnWindowResized()
{
	cfResource->aabbDirty_ = true;
}

void AABBGeneratorPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	if (!cfResource->aabbDirty_)
	{
		return;
	}

	Execute(commandBuffer);

	cfResource->aabbDirty_ = false;
}

void AABBGeneratorPipeline::Execute(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSets[gd->GetCurrentFrameIdx()],
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	gd->InsertDebugLabel(commandBuffer, "PipelineFrustumCulling", 0xff99ff99);

	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountZ)); // groupCountZ

	const VkBufferMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = gd->GetGraphicsFamily(),
		.dstQueueFamilyIndex = gd->GetGraphicsFamily(),
		.buffer = cfResource->aabbBuffer_.GetHandle(),
		.offset = 0,
		.size = cfResource->aabbBuffer_.GetSize() };
	Barrier::CreateBufferBarrier(commandBuffer, &barrier, 1u);
}

void AABBGeneratorPipeline::CreateDescriptor()
{
	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(&(cfResource->aabbBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // 1

	// Pool and layout
	descriptor.CreatePoolAndLayout(dsInfo, MAX_FRAMES_IN_FLIGHT, 1u);

	// Sets
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dsInfo.UpdateBuffer(&(cfUboBuffers[i]), 1);

		descriptor.CreateSet( dsInfo, &descriptorSets[i]);
	}
}
