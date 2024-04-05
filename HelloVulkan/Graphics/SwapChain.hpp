#pragma once

#include "GraphicsDevice.hpp"

class Window;

class SwapChain final
{
public:
	SwapChain(GraphicsDevice* graphicsDevice);
	~SwapChain();

	void ReCreate();

	[[nodiscard]] VkSwapchainKHR GetHandle() const { return swapChain; }
	[[nodiscard]] VkFormat GetImageFormat() const { return swapChainImageFormat; }
	[[nodiscard]] VkExtent2D GetExtent() const { return swapChainExtent; }
	[[nodiscard]] uint32_t GetImageCount() const { return swapChainImages.size(); }

	VkImageView GetImageView(int idx) const { return swapChainImageViews[idx]; };
private:
	friend class GraphicsDevice;
	GraphicsDevice* graphicsDevice;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent = {};
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;

	void DestroySwapChain();
	void CreateSwapChain();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(Window* window,const VkSurfaceCapabilitiesKHR& capabilities);
};
