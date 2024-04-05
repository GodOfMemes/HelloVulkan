#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include <Graphics/Mesh/MeshScenePODs.hpp>
#include <Graphics/Pipeline.hpp>

class MeshScene;
class ShadowResource;

class ShadowPipeline : public PipelineBase
{
public:
    ShadowPipeline(GraphicsDevice* gd, MeshScene* scene, ShadowResource* shadowResource);
    ~ShadowPipeline();

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
    void UpdateShadow(ShadowResource* shadowResource, glm::vec4 lightPosition);
private:
    BDA bda;
    MeshScene* scene;
    ShadowResource* shadowResource;

    Buffer indirectBuffer;
    std::vector<Buffer> shadowMapUBOBuffers;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    void CreateDescriptor();
};