#include "BRDFLUTPipeline.hpp"
#include <Graphics/Barrier.hpp>

BRDFLUTPipeline::BRDFLUTPipeline(GraphicsDevice* gd)
    : PipelineBase(gd, 
	{
		.type_ = PipelineType::Compute
	})
{
	outBuffer_.CreateBuffer(
		IBLConfig::LUTBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout, sizeof(BRDFLUTPushConst), VK_SHADER_STAGE_COMPUTE_BIT);
	CreateComputePipeline(SHADER_DIR + "IBL/BRDFLUT.comp");
}

BRDFLUTPipeline::~BRDFLUTPipeline()
{
    outBuffer_.Destroy();
}

void BRDFLUTPipeline::CreateLUT(Texture2D* outputLUT)
{
    // Need to use std::vector because std::array will cause stack overflow, 
	// probably because the length is too long.
	std::vector<float> lutData(IBLConfig::LUTBufferSize, 0);

	Execute();

	// Copy the buffer content to an image
	// TODO Find a way so that compute shader can write to an image
	outBuffer_.DownloadBufferData( lutData.data(), IBLConfig::LUTBufferSize);
	outputLUT->CreateImageFromData(
		lutData.data(),
		IBLConfig::LUTWidth,
		IBLConfig::LUTHeight,
		1, // Mipmap count
		1, // Layer count
		VK_FORMAT_R32G32_SFLOAT);
	outputLUT->CreateImageView(
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT);

	outputLUT->CreateDefaultSampler();
}

void BRDFLUTPipeline::Execute()
{
    gd->ImmediateSubmit([&](VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        BRDFLUTPushConst pc =
        {
            .width = IBLConfig::LUTWidth,
            .height = IBLConfig::LUTHeight,
            .sampleCount = IBLConfig::LUTSampleCount
        };
        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(BRDFLUTPushConst), &pc);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout,
            0, // firstSet
            1, // descriptorSetCount
            &descriptorSet_,
            0, // dynamicOffsetCount
            0); // pDynamicOffsets

        gd->InsertDebugLabel(commandBuffer, "BRDFLUTPipeline", 0xff99ffff);

        // Tell the GPU to do some compute
        vkCmdDispatch(commandBuffer, 
            static_cast<uint32_t>(IBLConfig::LUTWidth), // groupCountX
            static_cast<uint32_t>(IBLConfig::LUTHeight), // groupCountY
            1u); // groupCountZ
        
        // Set barrier
        constexpr VkMemoryBarrier2 barrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
            .dstAccessMask = VK_ACCESS_2_HOST_READ_BIT
        };
        Barrier::CreateMemoryBarrier(commandBuffer, &barrier, 1u);
    });
}

void BRDFLUTPipeline::CreateDescriptor()
{
    DescriptorInfo dsInfo;
	dsInfo.AddBuffer(&outBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	descriptor.CreatePoolAndLayout(dsInfo, 1u, 1u);
	descriptor.CreateSet(dsInfo, &descriptorSet_);
}
