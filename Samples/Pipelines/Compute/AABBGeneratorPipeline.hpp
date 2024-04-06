#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Pipeline.hpp"
#include "Resources/ClusterForwardResource.hpp"

/*
    Generate voxelized frustum for clustered forward
*/
class AABBGeneratorPipeline : public PipelineBase
{
public:
    AABBGeneratorPipeline(GraphicsDevice* gd, ClusterForwardResource* cfResource);
    ~AABBGeneratorPipeline();

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

    void SetClusterForwardUBO(ClusterForwardUBO& ubo)
    {
        cfUboBuffers[gd->GetCurrentFrameIdx()].UploadBufferData(&ubo, sizeof(ClusterForwardUBO));
    }
protected:
    void OnWindowResized() override;
private:
    ClusterForwardResource* cfResource;
    std::vector<Buffer> cfUboBuffers;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    void Execute(VkCommandBuffer commandBuffer);
    void CreateDescriptor();
};