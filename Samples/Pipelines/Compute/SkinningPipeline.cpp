#include "SkinningPipeline.hpp"
#include "Graphics/Barrier.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include <Graphics/Mesh/MeshScene.hpp>

SkinningPipeline::SkinningPipeline(GraphicsDevice* gd, MeshScene* scene)
    : PipelineBase(gd,
    {
        .type_ = PipelineType::Compute
    }),
    scene(scene)
{
    CreateDescriptor();
    CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);
    CreateComputePipeline(SHADER_DIR + "Skinning.comp");
}

void SkinningPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    Execute(commandBuffer, gd->GetCurrentFrameIdx());
}

void SkinningPipeline::Execute(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    auto frame = gd->GetCurrentFrame();
    TracyVkZoneC(frame.TracyContext, commandBuffer, "Skinning", tracy::Color::Lime);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSets[frameIndex],
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	gd->InsertDebugLabel(commandBuffer, "PipelineSkinning", 0xffff9999);

	constexpr float workgroupSize = 256.f;
	float vertexSize = static_cast<float>(scene->MeshSceneData_.vertices_.size());
	uint32_t groupSizeX = static_cast<uint32_t>(std::ceil(vertexSize / workgroupSize));
	vkCmdDispatch(commandBuffer, groupSizeX, 1, 1);

	VkBufferMemoryBarrier2 bufferBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, // Skinned vertex buffer is read in vertex shader
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = gd->GetGraphicsFamily(),
		.dstQueueFamilyIndex = gd->GetGraphicsFamily(),
		.buffer = scene->vertexBuffer_.GetHandle(),
		.offset = 0,
		.size = scene->vertexBuffer_.GetSize(),
	};
	Barrier::CreateBufferBarrier(commandBuffer, &bufferBarrier, 1u);
}

void SkinningPipeline::CreateDescriptor()
{
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
	DescriptorInfo dsInfo;

	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 0
	dsInfo.AddBuffer(&scene->boneIDBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	dsInfo.AddBuffer(&scene->boneWeightBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	dsInfo.AddBuffer(&scene->skinningIndicesBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 3
	dsInfo.AddBuffer(&scene->preSkinningVertexBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 4 Input
	dsInfo.AddBuffer(&scene->vertexBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 5 Output

	descriptor.CreatePoolAndLayout(dsInfo, MAX_FRAMES_IN_FLIGHT, 1u);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dsInfo.UpdateBuffer(&scene->boneMatricesBuffers_[i], 0);
		descriptor.CreateSet(dsInfo, &descriptorSets[i]);
	}
}
