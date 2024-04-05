#pragma once

#include <Graphics/Texture2D.hpp>
#include <Graphics/Pipeline.hpp>
#include <Graphics/GraphicsDevice.hpp>

/*
This applies tonemap to a color image and transfers it to a swapchain image
*/
class TonemapPipeline : public PipelineBase
{
public:
    TonemapPipeline(GraphicsDevice* gd, Texture2D* singledSampleColorImage);

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
protected:
    void OnWindowResized() override;
private:
    Texture2D* singleSampledColorImage;
    DescriptorInfo descriptorInfo;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    void CreateDescriptor();
};