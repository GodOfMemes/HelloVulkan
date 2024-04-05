#include "ClearPipeline.hpp"

ClearPipeline::ClearPipeline(GraphicsDevice* gd)
    : PipelineBase(gd, 
        PipelineConfig
        { 
            .type_ = PipelineType::GraphicsOnScreen
        })
{
    renderPass.CreateOnScreenColorOnlyRenderPass(RenderPassType::ColorClear);
    framebuffer.CreateResizeable(renderPass.GetHandle(), {}, IsOffscreen());
}

void ClearPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();
    TracyVkZoneC(frame.TracyContext, commandBuffer, "Clear", tracy::Color::DarkRed);
	const uint32_t swapchainImageIndex = frame.imageIndex;
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer(swapchainImageIndex));
	gd->InsertDebugLabel(commandBuffer, "PipelineClear", 0xff99ffff);
	vkCmdEndRenderPass(commandBuffer);
}
