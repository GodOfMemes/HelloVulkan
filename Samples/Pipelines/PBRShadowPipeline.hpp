#pragma once

#include "Graphics/Buffer.hpp"
#include "Graphics/GraphicsStructs.hpp"
#include "Graphics/Mesh/MeshScene.hpp"
#include "Resources/IBLResource.hpp"
#include "Resources/LightResource.hpp"
#include "Resources/ShadowResource.hpp"
#include "Resources/SharedResource.hpp"
#include <Graphics/Pipeline.hpp>
#include <Graphics/GraphicsDevice.hpp>

/*
    Render meshes using PBR materials, naive forward renderer with shadow mapping
*/
class PBRShadowPipeline : public PipelineBase
{
public:
    PBRShadowPipeline(
        GraphicsDevice* gd,
        MeshScene* scene, 
        LightResource* lightRes, 
        IBLResource* iblRes, 
        ShadowResource* shadowRes, 
        SharedResource* sharedRes, 
        MaterialType materialType, 
        uint8_t renderType = 0);
    ~PBRShadowPipeline();

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
    void SetShadowMapConfigUBO(ShadowMapUBO& ubo);
    void SetPBRPushConstants(const PBRPushConst& pbr) { pc = pbr; }
private:
    PBRPushConst pc;
    MeshScene* scene;
    Buffer bdaBuffer;
    LightResource* lightResource;
    IBLResource* iblResource;
    ShadowResource* shadowResource;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<Buffer> shadowMapConfigUBOBuffers;

    MaterialType materialType;
    VkDeviceSize materialOffset;
    uint32_t materialDrawCount;

    uint32_t alphaDiscard;

    void PrepareBDA();
    void CreateDescriptor();
    void CreateSpecializationConstants();
};