#include "LightCullingPipeline.hpp"
#include <Graphics/Barrier.hpp>

LightCullingPipeline::LightCullingPipeline(
	GraphicsDevice* ctx,
	LightResource* resourcesLight,
	ClusterForwardResource* resourcesCF) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::Compute
		}),
	resourcesLight_(resourcesLight),
	resourcesCF_(resourcesCF)
{
	Buffer::CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), MAX_FRAMES_IN_FLIGHT);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);

	std::string shaderFile = SHADER_DIR + "ClusteredForward/LightCulling.comp";
	//std::string shaderFile = AppConfig::ShaderFolder + "ClusteredForward/LightCullingBatch.comp";

	CreateComputePipeline(shaderFile);
}

LightCullingPipeline::~LightCullingPipeline()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
}

void LightCullingPipeline::ResetGlobalIndex()
{
	constexpr uint32_t zeroValue = 0u;
	resourcesCF_->globalIndexCountBuffers_[gd->GetCurrentFrameIdx()].UploadBufferData(&zeroValue, sizeof(uint32_t));
}

void LightCullingPipeline::SetClusterForwardUBO(ClusterForwardUBO& ubo)
{
	cfUBOBuffers_[gd->GetCurrentFrameIdx()].UploadBufferData(&ubo, sizeof(ClusterForwardUBO));
}

void LightCullingPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(gd->GetCurrentFrame().TracyContext, commandBuffer, "Light_Culling", tracy::Color::LawnGreen);
	Execute(commandBuffer, gd->GetCurrentFrameIdx());
}

void LightCullingPipeline::Execute(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
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

	gd->InsertDebugLabel(commandBuffer, "LightCullingPipeline", 0xff9999ff);

	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountZ)); // groupCountZ
	
	// Batched version
	/*vkCmdDispatch(commandBuffer,
		1,
		1,
		6);*/

	const VkBufferMemoryBarrier2 lightGridBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = gd->GetGraphicsFamily(),
		.dstQueueFamilyIndex = gd->GetGraphicsFamily(),
		.buffer = resourcesCF_->lightCellsBuffer_.GetHandle(),
		.offset = 0,
		.size = resourcesCF_->lightCellsBuffer_.GetSize()
	};
	const VkBufferMemoryBarrier2 lightIndicesBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = gd->GetGraphicsFamily(),
		.dstQueueFamilyIndex = gd->GetGraphicsFamily(),
		.buffer = resourcesCF_->lightIndicesBuffer_.GetHandle(),
		.offset = 0,
		.size = resourcesCF_->lightIndicesBuffer_.GetSize(),
	};
	const std::array<VkBufferMemoryBarrier2, 2> barriers =
	{
		lightGridBarrier,
		lightIndicesBarrier
	};
	Barrier::CreateBufferBarrier(commandBuffer, barriers.data(), static_cast<uint32_t>(barriers.size()));
}

void LightCullingPipeline::CreateDescriptor()
{
	const uint32_t frameCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(&(resourcesCF_->aabbBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 0
	dsInfo.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	dsInfo.AddBuffer(&(resourcesCF_->lightCellsBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 3
	dsInfo.AddBuffer(&(resourcesCF_->lightIndicesBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 4
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlag); // 5

	descriptor.CreatePoolAndLayout( dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(resourcesCF_->globalIndexCountBuffers_[i]), 2);
		dsInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 5);
		descriptor.CreateSet( dsInfo, &(descriptorSets_[i]));
	}
}