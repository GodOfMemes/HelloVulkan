#pragma once

#include "Graphics/Texture2D.hpp"
#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>

/*
	Offscreen pipeline to generate a cubemap from an HDR image
*/
class Equirect2CubePipeline final : public PipelineBase
{
public:
	Equirect2CubePipeline(GraphicsDevice* gd, const std::string& hdrFile);
	~Equirect2CubePipeline();

	void OffscreenRender(Texture2D* outputCubemap);

private:
	VkDescriptorSet descriptorSet_;
	Texture2D inputHDRImage_{gd};
	// TODO replace this below with framebuffer_
	VkFramebuffer cubeFramebuffer_;

	void InitializeHDRImage(const std::string& hdrFile);
	void InitializeCubemap(Texture2D* cubemap);
	void CreateCubemapViews(
		Texture2D* cubemap,
		std::vector<VkImageView>& cubemapViews);

	void CreateDescriptor();

	void CreateOffscreenGraphicsPipeline(
		VkRenderPass renderPass,
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline);

	void CreateFramebuffer(std::vector<VkImageView>& outputViews);
};
