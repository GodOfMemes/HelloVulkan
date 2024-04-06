#include "InfiniteGridPipeline.hpp"
#include "Defines.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Resources/SharedResource.hpp"

InfiniteGridPipeline::InfiniteGridPipeline(
	GraphicsDevice* ctx,
	SharedResource* resourcesShared,
	float yPosition,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.GetSampleCount(),
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	yPosition_(yPosition),
	shouldRender_(true)
{
	Buffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
	renderPass.CreateOffScreenRenderPass(renderBit, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(),
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		},
		IsOffscreen()
	);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout, sizeof(float), VK_SHADER_STAGE_VERTEX_BIT);
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "InfiniteGrid/Grid.vert",
			SHADER_DIR + "InfiniteGrid/Grid.frag",
		},
		&pipeline
	);
}

InfiniteGridPipeline::~InfiniteGridPipeline()
{

}

void InfiniteGridPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}
    
	TracyVkZoneC(gd->GetCurrentFrame().TracyContext, commandBuffer, "InfiniteGrid", tracy::Color::Lime);
	const uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());
	BindPipeline(commandBuffer);
	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(float), &yPosition_);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&descriptorSets_[frameIndex],
		0,
		nullptr);
	gd->InsertDebugLabel(commandBuffer, "InfiniteGridPipeline", 0xff99ff99);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void InfiniteGridPipeline::CreateDescriptor()
{
	constexpr uint32_t frameCount = MAX_FRAMES_IN_FLIGHT;
	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	descriptor.CreatePoolAndLayout( dsInfo, frameCount, 1u);
	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers[i]), 0);
		descriptor.CreateSet( dsInfo, &(descriptorSets_[i]));
	}
}