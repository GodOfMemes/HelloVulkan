#include "SwapChain.hpp"
#include "Core/Window.hpp"

SwapChain::SwapChain(GraphicsDevice* graphicsDevice)
	: graphicsDevice(graphicsDevice)
{
	CreateSwapChain();
}

SwapChain::~SwapChain()
{
	DestroySwapChain();
}

void SwapChain::ReCreate()
{
	auto old = swapChain;
	swapChain = VK_NULL_HANDLE;

	DestroySwapChain();

	oldSwapChain = old;

    CreateSwapChain();

	vkDestroySwapchainKHR(graphicsDevice->GetDevice(), oldSwapChain, nullptr);
	oldSwapChain = VK_NULL_HANDLE;
}

void SwapChain::DestroySwapChain()
{
	for(size_t i = 0; i < swapChainImages.size(); i++)
	{
		vkDestroyImageView(graphicsDevice->GetDevice(), swapChainImageViews[i], nullptr);
	}

	if(oldSwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(graphicsDevice->GetDevice(), oldSwapChain, nullptr);
	}

	if(swapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(graphicsDevice->GetDevice(), swapChain, nullptr);
	}
}

void SwapChain::CreateSwapChain()
{
	QueueFamilyIndices indices = graphicsDevice->FindQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };
	SwapChainSupportDetails swapChainSupport = graphicsDevice->QuerySwapChainSupport();
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = ChooseSwapExtent(graphicsDevice->window,swapChainSupport.Capabilities);

	uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if(swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

    VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = graphicsDevice->GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;
	createInfo.imageSharingMode = indices.GraphicsFamily != indices.PresentFamily ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = indices.GraphicsFamily != indices.PresentFamily ? 2 : 0;
	createInfo.pQueueFamilyIndices = indices.GraphicsFamily != indices.PresentFamily ? queueFamilyIndices : nullptr;

	if(vkCreateSwapchainKHR(graphicsDevice->GetDevice(), &createInfo, nullptr, &swapChain)!= VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(graphicsDevice->GetDevice(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(graphicsDevice->GetDevice(), swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	swapChainImageViews.resize(swapChainImages.size());
	for(size_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = swapChainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapChainImageFormat;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		if(vkCreateImageView(graphicsDevice->GetDevice(), &imageViewCreateInfo, nullptr, &swapChainImageViews[i])!= VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view!");
		}
	}
}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for(const auto& availableFormat : availableFormats)
	{
		if(availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
		   && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for(const auto& presentMode : availablePresentModes)
	{
		if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(Window* window, const VkSurfaceCapabilitiesKHR& capabilities)
{
	if(capabilities.currentExtent.width != UINT32_MAX) return capabilities.currentExtent;

	auto size = window->GetSize();
	VkExtent2D actualExtent = {static_cast<uint32_t>(size.x),static_cast<uint32_t>(size.y)};
	actualExtent.width = std::clamp(actualExtent.width,capabilities.minImageExtent.width,capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height,capabilities.minImageExtent.height,capabilities.maxImageExtent.height);
	return actualExtent;
}
