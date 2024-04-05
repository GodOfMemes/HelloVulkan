#include "ResolveMSPipeline.hpp"

ResolveMSPipeline::ResolveMSPipeline(
    GraphicsDevice* gd,
    SharedResource* sharedRes) :
    PipelineBase(gd, 
		{ .type_ = PipelineType::GraphicsOffScreen }
	)
{
	renderPass.CreateResolveMSRenderPass(
		0u,
		sharedRes->multiSampledColorImage_.GetSampleCount()
    );

	framebuffer.CreateResizeable(
		renderPass.GetHandle(),
		{
			&sharedRes->multiSampledColorImage_,
			&sharedRes->singleSampledColorImage_
		},
		IsOffscreen()
    );
}

void ResolveMSPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();
    TracyVkZoneC(frame.TracyContext, commandBuffer, "Multisample_Resolve", tracy::Color::GreenYellow);
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());
	gd->InsertDebugLabel(commandBuffer, "PipelineResolveMS", 0xffff99ff);
	vkCmdEndRenderPass(commandBuffer);
}
