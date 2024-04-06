#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Mesh/MeshScene.hpp"
#include "Resources/SharedResource.hpp"
#include <Graphics/Pipeline.hpp>

//LinePipeline

struct PointColor
{
	glm::vec4 position;
	glm::vec4 color;
};

class LinePipeline final : public PipelineBase
{
public:
	LinePipeline(
		GraphicsDevice* ctx,
		SharedResource* resShared,
		MeshScene* scene,
		uint8_t renderBit = 0u);
	~LinePipeline();

	void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
	void SetFrustum(CameraUBO& camUBO);

    bool shouldRender_;
private:

	// Bounding box rendering
	MeshScene* scene_;
	std::vector<PointColor> lineDataArray_;
	std::vector<Buffer> lineBuffers_;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

	// Camera frustum rendering
	std::vector<PointColor> frustumDataArray_;

    void CreateDescriptor();
	void CreateBuffers();

	void ProcessScene();
	void AddBox(const glm::mat4& mat, const glm::vec3& size, const glm::vec4& color);
	void AddLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);
	void UploadLinesToBuffer(uint32_t frameIndex);

	void InitFrustumLines();
	void UpdateFrustumLine(int index, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);
	void UploadFrustumLinesToBuffer(uint32_t frameIndex);
};