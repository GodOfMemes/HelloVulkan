#include "LightRenderPipeline.hpp"
#include "Graphics/GraphicsDevice.hpp"

LightRenderPipeline::LightRenderPipeline(
    GraphicsDevice* gd, 
    LightResource* lightRes, 
    SharedResource* sharedRes,
    uint8_t renderType) :
    PipelineBase(gd, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = sharedRes->multiSampledColorImage_.GetSampleCount(),
			.depthTest_ = true,
			.depthWrite_ = false // To "blend" the circles
		}
	), // Offscreen rendering
	lightResource(lightRes),
	shouldRender(true)
{
	Buffer::CreateMultipleUniformBuffers(gd,cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
	renderPass.CreateOffScreenRenderPass( renderType, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(),
		{
			&sharedRes->multiSampledColorImage_,
			&sharedRes->depthImage_
		},
		IsOffscreen()
	);
	CreateDescriptor();
	CreatePipelineLayout( descriptor.GetLayout(), &pipelineLayout);
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "LightOrb.vert",
			SHADER_DIR + "LightOrb.frag",
		},
		&pipeline
	);
}

void LightRenderPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    if (!shouldRender)
	{
		return;
	}

    auto frame = gd->GetCurrentFrame();

	TracyVkZoneC(frame.TracyContext, commandBuffer, "Lights", tracy::Color::AliceBlue);
	uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());
	BindPipeline(commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&descriptorSets[frameIndex],
		0,
		nullptr);
	gd->InsertDebugLabel(commandBuffer, "PipelineLightRender", 0xff99ff99);
	vkCmdDraw(
		commandBuffer, 
		6, // Draw a quad
		lightResource->GetLightCount(), 
		0, 
		0);
	vkCmdEndRenderPass(commandBuffer);
}

void LightRenderPipeline::CreateDescriptor()
{

	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // 0
	dsInfo.AddBuffer(lightResource->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 1

	// Create pool and layout
	descriptor.CreatePoolAndLayout( dsInfo, MAX_FRAMES_IN_FLIGHT, 1u);

	// Create sets
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dsInfo.UpdateBuffer(&cameraUBOBuffers[i], 0);
		descriptor.CreateSet(dsInfo, &descriptorSets[i]);
	}
}
