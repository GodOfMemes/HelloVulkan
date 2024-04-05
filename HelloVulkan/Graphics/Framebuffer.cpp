#include "Framebuffer.hpp"
#include "SwapChain.hpp"

void Framebuffer::CreateResizeable(
	VkRenderPass renderPass,
	const std::vector<Texture2D*>& attachmentImages,
	bool offscreen)
{
	_resizeable = true;
	_offscreen = offscreen;
	_attachmentImages = attachmentImages;
	_frameBufferCount = _offscreen ? 1u : static_cast<uint32_t>(_graphicsDevice->GetSwapChain()->GetImageCount());
	_frameBuffers.resize(_frameBufferCount);

	if (_offscreen && _attachmentImages.empty())
	{
		std::cerr << "Need at least one image attachment to create a framebuffer\n";
	}

	// Create info
	_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		// Need to set these four in Recreate()
		/*.attachmentCount =,
		.pAttachments =,
		.width = w,
		.height = h,*/
		.layers = 1
	};

	// Create framebuffer in this function
	Recreate();
}

void Framebuffer::CreateUnresizeable(
    VkRenderPass renderPass,
	const std::vector<VkImageView>& attachments,
	uint32_t width,
	uint32_t height)
{
	_resizeable = false;

	_info =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = renderPass,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.width = width,
		.height = height,
		.layers = 1u
	};
	_frameBuffers.resize(1);
	VK_CHECK(vkCreateFramebuffer(_graphicsDevice->GetDevice(), &_info, nullptr, _frameBuffers.data()));
}

void Framebuffer::Destroy()
{
	for (size_t i = 0; i < _frameBuffers.size(); ++i)
	{
		vkDestroyFramebuffer(_graphicsDevice->GetDevice(), _frameBuffers[i], nullptr);
		_frameBuffers[i] = nullptr;
	}
	// NOTE Don't clear because we may recreate
	//_frameBuffers.clear();
}

VkFramebuffer Framebuffer::GetFramebuffer() const
{
	return GetFramebuffer(0);
}

VkFramebuffer Framebuffer::GetFramebuffer(size_t index) const
{
	if (index < _frameBuffers.size())
	{
		return _frameBuffers[index];
	}
	return VK_NULL_HANDLE;
}

void Framebuffer::Recreate()
{
	if (!_resizeable)
	{
		std::cerr << "Cannot resize framebuffer\n";
	}

	const size_t swapchainImageCount = _offscreen ? 0 : 1;
	const size_t attachmentLength = _attachmentImages.size() + swapchainImageCount;

	if (attachmentLength <= 0)
	{
		return;
	}

	std::vector<VkImageView> attachments(attachmentLength, VK_NULL_HANDLE);
	for (size_t i = 0; i < _attachmentImages.size(); ++i)
	{
		attachments[i + swapchainImageCount] = _attachmentImages[i]->GetView();
	}

	_info.width = _graphicsDevice->GetSwapChain()->GetExtent().width;
	_info.height = _graphicsDevice->GetSwapChain()->GetExtent().height;

	for (size_t i = 0; i < _frameBufferCount; ++i)
	{
		if (swapchainImageCount > 0)
		{
			attachments[0] = _graphicsDevice->GetSwapChain()->GetImageView(i);
		}

		_info.attachmentCount = static_cast<uint32_t>(attachmentLength);
		_info.pAttachments = attachments.data();
		VK_CHECK(vkCreateFramebuffer(_graphicsDevice->GetDevice(), &_info, nullptr, &_frameBuffers[i]));
	}
}