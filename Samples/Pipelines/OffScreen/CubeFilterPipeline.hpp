#pragma once

#include <Graphics/Pipeline.hpp>
#include <Graphics/GraphicsDevice.hpp>

class Texture2D;

enum class CubeFilterType : uint8_t
{
	// Iradiance / diffuse map
	Diffuse = 0u,

	// Prefilter / specular map (mipmapped)
	Specular = 1u, 
};

/*
Offscreen pipeline to create specular map and diffuse map.
This class actually has two graphics pipelines.
*/
class CubeFilterPipeline final : public PipelineBase
{
public:
	CubeFilterPipeline(GraphicsDevice* gd, Texture2D* inputCubemap);
	~CubeFilterPipeline();

	void OffscreenRender(
		Texture2D* outputCubemap,
		CubeFilterType filterType);

private:

	VkDescriptorSet descriptorSet_;
	VkSampler inputCubemapSampler_; // A sampler for the input cubemap

	// Two pipelines for each of diffuse and specular maps
	std::vector<VkPipeline> graphicsPipelines_;

	void CreateDescriptor(Texture2D* inputCubemap);

	void InitializeOutputCubemap( 
		Texture2D* outputDiffuseCubemap,
		uint32_t numMipmap,
		uint32_t inputCubeSideLength);

	void CreateOutputCubemapViews(
		Texture2D* outputCubemap,
		std::vector<std::vector<VkImageView>>& outputCubemapViews,
		uint32_t numMip);

	void CreateOffscreenGraphicsPipeline(
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		uint32_t viewportWidth,
		uint32_t viewportHeight,
		VkPipeline* pipeline);
};