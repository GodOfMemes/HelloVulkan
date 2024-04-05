#include "FinishPipeline.hpp"
FinishPipeline::FinishPipeline(GraphicsDevice* gd)
    : PipelineBase(gd,
		{
			.type_ = PipelineType::GraphicsOnScreen
		})
{
	renderPass.CreateOnScreenColorOnlyRenderPass(
		// Present swapchain image 
		RenderPassType::ColorPresent);
	framebuffer.CreateResizeable( renderPass.GetHandle(), {}, IsOffscreen());
}


void FinishPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();
    TracyVkZoneC(frame.TracyContext, commandBuffer, "Finish", tracy::Color::MediumAquamarine);
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer(frame.imageIndex));
	gd->InsertDebugLabel(commandBuffer, "PipelineFinish", 0xff99ffff);
	vkCmdEndRenderPass(commandBuffer);
}
