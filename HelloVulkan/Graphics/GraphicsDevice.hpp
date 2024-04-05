#pragma once

#include "Minimal.hpp"
#include "GraphicsStructs.hpp"
#include <volk.h> //<vulkan/vulkan.h> 
#include <vk_mem_alloc.h>
#include <tracy/TracyVulkan.hpp>

#define VK_CHECK(value) VK_CHECKF(value,value)
#define VK_CHECKF(value, fmt) do { if(value) { std::cout << "Detected Error: " << fmt << " in " <<  __FILE__ << " at " << __LINE__ << std::endl; } } while(0);

class Window;
class SwapChain;
class PipelineBase;
class ResourceBase;

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2u;
static_assert(MAX_FRAMES_IN_FLIGHT > 0u, "MAX_FRAMES_IN_FLIGHT must be greater than 0");

struct GraphicsSettings
{
    VkSampleCountFlagBits MSAACount = VK_SAMPLE_COUNT_4_BIT;
    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
};

struct FrameData
{
    VkSemaphore ImageAvailableSemaphore;
    VkSemaphore RenderFinishedSemaphore;
    VkFence InFlightFence;
    VkCommandBuffer CmdBuffer;
    TracyVkCtx TracyContext;
	uint32_t imageIndex;

    void Destroy(VkDevice device)
    {
        vkDestroySemaphore(device,ImageAvailableSemaphore,nullptr);
        vkDestroySemaphore(device,RenderFinishedSemaphore,nullptr);
        vkDestroyFence(device,InFlightFence,nullptr);
        if(TracyContext)
        {
            TracyVkDestroy(TracyContext);
        }
    }
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;

	[[nodiscard]] bool IsComplete() const
	{
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};


struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};

class GraphicsDevice final
{
public:
	GraphicsDevice(Window* window, const GraphicsSettings& settings);
	~GraphicsDevice();

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmdBuffer)>&& callback);

	VkFormat FindDepthFormat();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	QueueFamilyIndices FindQueueFamilies() { return FindQueueFamilies(physicalDevice); }
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice pd);
	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice pd, VkSurfaceKHR surface);

	SwapChainSupportDetails QuerySwapChainSupport() { return QuerySwapChainSupport(physicalDevice); }
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice pd);
	static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice pd, VkSurfaceKHR surface);

	VkResult CreateSemaphore(VkSemaphore* semaphore) const;
	VkResult CreateFence(VkFence* fence) const;
	VkResult CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer* commandBuffer) const;
	VkResult CreateCommandPool(uint32_t family, VkCommandPool* pool) const;

	[[nodiscard]] VmaAllocator GetAllocator() const { return allocator; }
 	[[nodiscard]] VkInstance GetInstance() const { return instance; }
	[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	[[nodiscard]] VkDevice GetDevice() const { return device; }
	[[nodiscard]] VkQueue GetGraphicsQueue() const { return graphicsQueue; }
	[[nodiscard]] VkQueue GetPresentQueue() const { return presentQueue; }
	[[nodiscard]] VkSurfaceKHR GetSurface() const { return surface; }
	[[nodiscard]] SwapChain* GetSwapChain() const { return swapChain; }
	[[nodiscard]] VkCommandPool GetCommandPool() const { return commandPool; }

	[[nodiscard]] VkSampleCountFlagBits GetSampleCount() const { return sampleCount;}
	[[nodiscard]] uint32_t GetCurrentFrameIdx() const { return currentFrameIdx; }
	[[nodiscard]] VkFormat GetImageFormat() const;
	[[nodiscard]] VkFormat GetDepthFormat() const;

	//[[nodiscard]] VkCommandBuffer GetCurrentRenderCommand() const { return commandBuffers[currentFrame]; }
	[[nodiscard]] FrameData GetCurrentFrame() const { return Frame[currentFrameIdx]; }

	[[nodiscard]] uint32_t GetGraphicsFamily() const { return selectedFamilies.GraphicsFamily.value();}
	[[nodiscard]] uint32_t GetPresentFamily() const { return selectedFamilies.PresentFamily.value();}

	[[nodiscard]] static VkDescriptorSetLayoutBinding CreateDescriptorSetBinding(uint32_t binding,VkDescriptorType type, VkShaderStageFlags stageFlags);

	void SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name) const;
	void InsertDebugLabel(VkCommandBuffer commandBuffer, const char* label, uint32_t colorRGBA) const;

	template<class T, class... U>
	requires (std::is_base_of<PipelineBase, T>::value)
	T* AddPipeline(U&&... u)
	{
		// Create std::unique_ptr of Pipeline
		std::unique_ptr<T> pipeline = std::make_unique<T>(std::forward<U>(u)...);
		T* ptr = pipeline.get();
		pipelines.push_back(std::move(pipeline)); // Put it in std::vector
		return ptr;
	}

	template<class T, class... U>
	requires (std::is_base_of<ResourceBase, T>::value)
	T* AddResources(U&&... u)
	{
		// Create std::unique_ptr of Resources
		std::unique_ptr<T> resource = std::make_unique<T>(std::forward<U>(u)...);
		T* ptr = resource.get();
		resources.push_back(std::move(resource)); // Put it in std::vector
		return ptr;
	}
private:
	friend class Application;
	friend class SwapChain;

	Window* window;
    GraphicsSettings Settings;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VmaAllocator allocator;
	QueueFamilyIndices selectedFamilies;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	SwapChain* swapChain;

	VkPhysicalDeviceMemoryProperties memProps;
	VkCommandPool commandPool;
	FrameData Frame[MAX_FRAMES_IN_FLIGHT];
	VkSampleCountFlagBits sampleCount;
	VkFormat depthFormat;

	std::vector<std::unique_ptr<PipelineBase>> pipelines;
	std::vector<std::unique_ptr<ResourceBase>> resources;

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
	std::vector<const char*> validationLayers =
	{
			"VK_LAYER_KHRONOS_validation"
	};
#endif

	uint32_t currentFrameIdx = 0u;

	std::vector<const char*> deviceExtensions =
	{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE3_EXTENSION_NAME,
			VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
	};

	VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressEnabledFeatures = {};
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {};
	VkPhysicalDeviceVulkan13Features features13 = {};
	VkPhysicalDeviceVulkan11Features features11 = {};
	VkPhysicalDeviceFeatures2 features2 = {};
	VkPhysicalDeviceFeatures features = {};

	void DrawFrame(std::function<void(VkCommandBuffer cmdBuffer)>&& renderCallback);
	void HandleResize();
	void WaitForIdle();

	void CreateInstance(const std::string& applicationTitle);
	void SelectPhysicalDevice(VkSampleCountFlagBits requestedSampleCount);
	void SetupDeviceFeatures();
	void CreateLogicalDevice();
	void SetupCommandBuffers();

	void CheckValidationLayerSupport();
	bool IsDeviceSuitable(VkPhysicalDevice pd);
	VkSampleCountFlagBits GetRequestedSampleCount(VkPhysicalDevice pd,VkSampleCountFlagBits requestedSampleCount);
};