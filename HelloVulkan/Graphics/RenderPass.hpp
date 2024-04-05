#pragma once

#include "GraphicsDevice.hpp"

enum RenderPassType : uint8_t
{
    None = 0,

    // Clear color attachment
	ColorClear = 0x01,

	// Clear depth attachment
	DepthClear = 0x02,

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	ColorShaderReadOnly = 0x04,

	// Transition color attachment to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	ColorPresent = 0x08,

	// Transition depth attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	DepthShaderReadOnly = 0x10,
};

class RenderPass
{
public:
    RenderPass(GraphicsDevice* graphicsDevice) 
        : _graphicsDevice(graphicsDevice) {}

    ~RenderPass() { Destroy(); }

    void CreateOnScreenRenderPass(
		uint8_t renderPassType = RenderPassType::None,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateDepthOnlyRenderPass(
		uint8_t renderPassType = RenderPassType::None,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateResolveMSRenderPass(
		uint8_t renderPassType = RenderPassType::None,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOnScreenColorOnlyRenderPass(
		uint8_t renderPassType = RenderPassType::None,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOffScreenRenderPass(
		uint8_t renderPassType = RenderPassType::None,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	void CreateOffScreenCubemapRenderPass(
		VkFormat cubeFormat,
		uint8_t renderPassType = RenderPassType::None,
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	// There's no EndRenderPass() function because you can just call vkCmdEndRenderPass(commandBuffer);
	void BeginRenderPass(
		VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer);

	void BeginRenderPass(
		VkCommandBuffer commandBuffer,
		VkFramebuffer framebuffer,
		VkExtent2D size);

	void BeginCubemapRenderPass(
		VkCommandBuffer commandBuffer, 
		VkFramebuffer framebuffer,
		uint32_t cubeSideLength);

	void Destroy();

	VkRenderPass GetHandle() const { return _handle;}
private:
    GraphicsDevice* _graphicsDevice;
    VkRenderPass _handle = nullptr;
    uint8_t _renderPassType;

    // Cache for starting the render pass
	std::vector<VkClearValue> clearValues_;
	VkRenderPassBeginInfo beginInfo_;

	void CreateBeginInfo();
};