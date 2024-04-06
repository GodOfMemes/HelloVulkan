#pragma once

#include "Graphics/Buffer.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/GraphicsStructs.hpp"
#include "Graphics/Mesh/MeshScene.hpp"
#include "Graphics/Mesh/MeshScenePODs.hpp"
#include "Graphics/Pipeline.hpp"
#include "Resources/ClusterForwardResource.hpp"
#include "Resources/IBLResource.hpp"
#include "Resources/LightResource.hpp"
#include "Resources/SharedResource.hpp"

/*
Render meshes using PBR materials, clustered forward renderer
*/

class PBRClusterForwardPipeline : public PipelineBase
{
public:
	PBRClusterForwardPipeline(
		GraphicsDevice* gd,
		MeshScene* scene,
		LightResource* lights,
		ClusterForwardResource* resourcesCF,
		IBLResource* resourcesIBL,
		SharedResource* resourcesShared,
		MaterialType materialType,
		uint8_t renderBit = 0u);
	~PBRClusterForwardPipeline();

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

	void SetPBRPushConstants(const PBRPushConst& pbrPC) { pc_ = pbrPC; };

	void SetClusterForwardUBO(ClusterForwardUBO& ubo)
	{
		cfUBOBuffers_[gd->GetCurrentFrameIdx()].UploadBufferData( &ubo, sizeof(ClusterForwardUBO));
	}

private:
	void PrepareBDA();
	void CreateDescriptor();
	void CreateSpecializationConstants();

private:
	PBRPushConst pc_;
	ClusterForwardResource* resourcesCF_;
	LightResource* resourcesLight_;
	IBLResource* resourcesIBL_;
	std::vector<Buffer> cfUBOBuffers_;
	Buffer bdaBuffer_;
	MeshScene* scene_;
	std::vector<VkDescriptorSet> descriptorSets_;

	// Material pass
	MaterialType materialType_;
	VkDeviceSize materialOffset_;
	uint32_t materialDrawCount_;

	// Specialization constants
	uint32_t alphaDiscard_;

};