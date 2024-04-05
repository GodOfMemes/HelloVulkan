#pragma once

#include <Graphics/Pipeline.hpp>
#include <Graphics/GraphicsDevice.hpp>

class MeshScene;

class SkinningPipeline : public PipelineBase
{
public:
    SkinningPipeline(GraphicsDevice* gd, MeshScene* scene);

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
private:
    MeshScene* scene;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    void Execute(VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void CreateDescriptor();
};