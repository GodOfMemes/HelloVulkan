#include "TonemapPipeline.hpp"

TonemapPipeline::TonemapPipeline(GraphicsDevice* gd, Texture2D* singledSampleColorImage)
    : PipelineBase(gd,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}),
	// Need to store a pointer for window resizing
	singleSampledColorImage(singledSampleColorImage)
{
	renderPass.CreateOnScreenColorOnlyRenderPass();
	framebuffer.CreateResizeable(renderPass.GetHandle(), {}, IsOffscreen());
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "FullscreenTriangle.vert",
			SHADER_DIR + "Tonemap.frag",
		},
		&pipeline);
}

void TonemapPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();
    TracyVkZoneC(frame.TracyContext, commandBuffer, "Tonemap", tracy::Color::MediumVioletRed);
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer(frame.imageIndex));
	BindPipeline(commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&descriptorSets[gd->GetCurrentFrameIdx()],
		0,
		nullptr);
	gd->InsertDebugLabel(commandBuffer, "PipelineTonemap", 0xffff9999);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void TonemapPipeline::OnWindowResized()
{
    PipelineBase::OnWindowResized();

    descriptorInfo.UpdateImage(singleSampledColorImage, 0);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		descriptor.UpdateSet(descriptorInfo, &descriptorSets[i]);
	}
}

void TonemapPipeline::CreateDescriptor()
{
	descriptorInfo.AddImage(nullptr);

	// Pool and layout
	descriptor.CreatePoolAndLayout( descriptorInfo, MAX_FRAMES_IN_FLIGHT, 1u);

    descriptorInfo.UpdateImage(singleSampledColorImage, 0);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
        descriptor.AllocateSet(&descriptorSets[i]);
		descriptor.UpdateSet(descriptorInfo, &descriptorSets[i]);
	}
}
