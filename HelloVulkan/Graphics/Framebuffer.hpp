#pragma once

#include "GraphicsDevice.hpp"
#include "Texture2D.hpp"

class Framebuffer
{
public:
    Framebuffer(GraphicsDevice* graphicsDevice) 
        : _graphicsDevice(graphicsDevice) {}

    ~Framebuffer() { Destroy(); }

    void Destroy();

    // Can be recreated/resized
	void CreateResizeable(
        VkRenderPass renderPass,
		const std::vector<Texture2D*>& attachmentImage,
		bool offscreen);

	// Cannot be recreated/resized
	void CreateUnresizeable(
		VkRenderPass renderPass,
		const std::vector<VkImageView>& attachments,
		uint32_t width,
		uint32_t height);

	[[nodiscard]] VkFramebuffer GetFramebuffer() const;
	[[nodiscard]] VkFramebuffer GetFramebuffer(size_t index) const;
	void Recreate();
private:
    GraphicsDevice* _graphicsDevice;
    std::vector<VkFramebuffer> _frameBuffers;
    std::vector<Texture2D*> _attachmentImages;
    VkFramebufferCreateInfo _info;
    uint32_t _frameBufferCount = 0;
    bool _offscreen, _resizeable;
};