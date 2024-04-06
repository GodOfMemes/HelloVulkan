#pragma once

#include "Graphics/Buffer.hpp"
#include "Graphics/GraphicsStructs.hpp"
#include "Graphics/Mesh/MeshScene.hpp"
#include "Resources/IBLResource.hpp"
#include "Resources/LightResource.hpp"
#include "Resources/SharedResource.hpp"
#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>

/*
Render a scene using PBR materials, naive forward renderer, and bindless textures
*/

class PBRBindlessPipeline final : public PipelineBase
{
public:
	PBRBindlessPipeline(GraphicsDevice* ctx,
		MeshScene* scene,
		LightResource* resourcesLight,
		IBLResource* resourcesIBL,
		SharedResource* resourcesShared,
		bool useSkinning,
		uint8_t renderBit = 0u);
	 ~PBRBindlessPipeline();

	 void SetPBRPushConstants(const PBRPushConst& pbrPC) { pc_ = pbrPC; };

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;

private:
	void PrepareBDA();
	void CreateDescriptor();

	bool useSkinning_;
	MeshScene* scene_;
	LightResource* resourcesLight_;
	IBLResource* resourcesIBL_;
	PBRPushConst pc_;
	Buffer bdaBuffer_;
	std::vector<VkDescriptorSet> descriptorSets_;
};