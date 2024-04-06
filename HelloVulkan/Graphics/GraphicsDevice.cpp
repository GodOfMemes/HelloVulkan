#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "GraphicsDevice.hpp"
#include "Core/Window.hpp"
#include "SwapChain.hpp"
#include "GLFW/glfw3.h"
#include <set>
#include "Pipeline.hpp"
#include "Resource.hpp"

#include <glslang/Include/glslang_c_interface.h>

#ifdef DEBUG
/*VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT");
	if(func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT");
	if(func != nullptr)
		func(instance,debugMessenger,pAllocator);
}

void CmdInsertDebugUtilsLabelEXT(VkInstance instance,VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
	auto func = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
	if(func != nullptr)
		func(commandBuffer,pLabelInfo);
}

VkResult SetDebugUtilsObjectNameEXT(VkInstance instance,VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

	if(func != nullptr)
	{
		return func(device,pNameInfo);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}*/

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
{
	if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		std::cout << "[Vulkan] Validation: "  << pCallbackData->pMessage << std::endl;
	}
	else
	{
		std::cout << "[Vulkan]: "  << pCallbackData->pMessage << std::endl;
	}
	return VK_FALSE;
}
#endif

GraphicsDevice::GraphicsDevice(Window* window, const GraphicsSettings& settings)
	: window(window), Settings(settings)
{
	const VkResult res = volkInitialize();
	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Volk Cannot be initialized");
	}

	CreateInstance(window->GetTitle());

	glslang_initialize_process();	

    surface = window->CreateSurface(instance);
	SelectPhysicalDevice(settings.MSAACount);
	CreateLogicalDevice();

	VmaVulkanFunctions vulkanFunctions =
	{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
	};

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	allocatorInfo.pVulkanFunctions = (const VmaVulkanFunctions*)&vulkanFunctions;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	if(vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vma allocator!");
	}

	SetupCommandBuffers();

	swapChain = new SwapChain(this);
	depthFormat = FindDepthFormat();

	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		FrameData& frameData = Frame[i];
		
		if(CreateSemaphore(&frameData.ImageAvailableSemaphore) != VK_SUCCESS ||
		   CreateSemaphore(&frameData.RenderFinishedSemaphore)!= VK_SUCCESS ||
		   CreateFence(&frameData.InFlightFence)!= VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization objects for a frame!");
		}

		frameData.TracyContext = TracyVkContext(physicalDevice, device, graphicsQueue, frameData.CmdBuffer);
	}
}

GraphicsDevice::~GraphicsDevice()
{
	WaitForIdle();

	for(auto& pip : pipelines) { pip.reset(); }
	for(auto& res : resources) { res.reset(); }

	delete swapChain;

	for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		Frame[i].Destroy(device);
	}
	
	vmaDestroyAllocator(allocator);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
#ifdef DEBUG
	vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
	vkDestroyInstance(instance, nullptr);
}

void GraphicsDevice::ImmediateSubmit(std::function<void(VkCommandBuffer cmdBuffer)>&& callback)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	callback(commandBuffer);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkFormat GraphicsDevice::FindDepthFormat()
{
	return FindSupportedFormat(
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat GraphicsDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                        VkFormatFeatureFlags features)
{
	for(auto format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(GetPhysicalDevice(),format,&props);

		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

QueueFamilyIndices GraphicsDevice::FindQueueFamilies(VkPhysicalDevice pd)
{
	return FindQueueFamilies(pd, surface);
}

QueueFamilyIndices GraphicsDevice::FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices{};
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,&queueFamilyCount,nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,&queueFamilyCount,queueFamilies.data());

	int i = 0;
	for(const auto& queueFamily : queueFamilies)
	{
		if(queueFamily.queueCount > 0 && queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
		{
			indices.GraphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice,i,surface,&presentSupport);

		if(presentSupport)
		{
			indices.PresentFamily = i;
		}

		if(indices.IsComplete())
		{
			break;
		}

		i++;
	}
	return indices;
}

SwapChainSupportDetails GraphicsDevice::QuerySwapChainSupport(VkPhysicalDevice pd)
{
	return QuerySwapChainSupport(pd,surface);
}

SwapChainSupportDetails GraphicsDevice::QuerySwapChainSupport(VkPhysicalDevice pd, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &details.Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &formatCount, nullptr);
	if(formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, nullptr);
	if(presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, details.PresentModes.data());
	}

	return details;
}

VkResult GraphicsDevice::CreateSemaphore(VkSemaphore* semaphore) const
{
	const VkSemaphoreCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	return vkCreateSemaphore(device, &ci, nullptr, semaphore);
}

VkResult GraphicsDevice::CreateFence(VkFence* fence) const
{
	const VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	return vkCreateFence(device, &fenceInfo, nullptr, fence);
}

VkResult GraphicsDevice::CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer* commandBuffer) const
{
	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	return vkAllocateCommandBuffers(device, &allocInfo, commandBuffer);
}

VkResult GraphicsDevice::CreateCommandPool(uint32_t family, VkCommandPool* pool) const
{
	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = family
	};

	return vkCreateCommandPool(device, &cpi, nullptr, pool);
}

VkFormat GraphicsDevice::GetImageFormat() const
{
	return swapChain->GetImageFormat();
}

VkFormat GraphicsDevice::GetDepthFormat() const 
{ 
	return depthFormat;
}

void GraphicsDevice::SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name) const
{
#ifdef DEBUG
	const VkDebugUtilsObjectNameInfoEXT nameInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = objType,
		.objectHandle = reinterpret_cast<uint64_t>(objectHandle),
		.pObjectName = name
	};
	VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &nameInfo));
#endif
}

void GraphicsDevice::InsertDebugLabel(VkCommandBuffer commandBuffer, const char* label, uint32_t colorRGBA) const
{
#ifdef DEBUG
	const VkDebugUtilsLabelEXT utilsLabel = {
	 .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
	 .pNext = nullptr,
	 .pLabelName = label,
	 .color = {float((colorRGBA >> 0) & 0xff) / 255.0f,
				float((colorRGBA >> 8) & 0xff) / 255.0f,
				float((colorRGBA >> 16) & 0xff) / 255.0f,
				float((colorRGBA >> 24) & 0xff) / 255.0f},
	};
	vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &utilsLabel);
#endif
}

void GraphicsDevice::DrawFrame(std::function<void(VkCommandBuffer cmdBuffer)>&& renderCallback)
{
	FrameData& currentFrame = Frame[currentFrameIdx];
	{
		ZoneScopedNC("WaitForFences", tracy::Color::GreenYellow);
		vkWaitForFences(device,1,&currentFrame.InFlightFence,VK_TRUE,UINT64_MAX);
	}

	uint32_t imageIndex;
	VkResult result;
	{
		ZoneScopedNC("AcquireNextImageKHR", tracy::Color::PaleGreen);
		result = vkAcquireNextImageKHR(device,swapChain->swapChain,UINT64_MAX,currentFrame.ImageAvailableSemaphore,VK_NULL_HANDLE,&imageIndex);
		if(result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapChain->ReCreate();
			return;
		}
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire next image!");
		}
	}
	currentFrame.imageIndex = imageIndex;

	{
		ZoneScopedNC("ResetFences_ResetCommandBuffer", tracy::Color::Orange);
		vkResetFences(device,1,&currentFrame.InFlightFence);
		vkResetCommandBuffer(currentFrame.CmdBuffer,0);
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_CHECKF(vkBeginCommandBuffer(currentFrame.CmdBuffer,&beginInfo), "Failed to begin recording command buffer!")
	{
		//ZoneScopedNC("RecordCommandBuffer", tracy::Color::OrangeRed);
		TracyVkZoneC(currentFrame.TracyContext, currentFrame.CmdBuffer, "Render", tracy::Color::OrangeRed);
		renderCallback(currentFrame.CmdBuffer);
	}
	TracyVkCollect(currentFrame.TracyContext, currentFrame.CmdBuffer);
	VK_CHECKF(vkEndCommandBuffer(currentFrame.CmdBuffer), "Failed to record command buffer!")

	{
		ZoneScopedNC("QueueSubmit", tracy::Color::VioletRed);
		VkPipelineStageFlags waitMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &currentFrame.ImageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = waitMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &currentFrame.CmdBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &currentFrame.RenderFinishedSemaphore;

		if(vkQueueSubmit(graphicsQueue,1,&submitInfo,currentFrame.InFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}
	}

	{
		ZoneScopedNC("QueuePresentKHR", tracy::Color::VioletRed1);
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &currentFrame.RenderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain->swapChain;
		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(graphicsQueue,&presentInfo);

		if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			HandleResize();
		}
		else if(result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}
	}
	currentFrameIdx = (currentFrameIdx + 1) % MAX_FRAMES_IN_FLIGHT;
	FrameMark;
}

void GraphicsDevice::HandleResize()
{
	WaitForIdle();
	auto size = window->GetSize();
	while(size.x == 0 || size.y == 0)
	{
		size = window->GetSize();
		glfwWaitEvents();
	}
	swapChain->ReCreate();

	for(auto& res : resources) 
	{ 
		if(res && res->recreateWhenSwapChanges)
		{
			res->Create();
		}  
	}

	for(auto& pip : pipelines) 
	{ 
		if(pip)
		{
			pip->OnWindowResized();
		}  
	}
}

void GraphicsDevice::WaitForIdle()
{
	vkDeviceWaitIdle(device);
}

void GraphicsDevice::CreateInstance(const std::string& applicationTitle)
{
#ifdef DEBUG
	CheckValidationLayerSupport();
#endif

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = applicationTitle.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.pNext = nullptr;

#ifdef DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = DebugCallback;

	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	instanceCreateInfo.pNext = &debugCreateInfo;
#endif

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef DEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	instanceCreateInfo.enabledExtensionCount = extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    if(vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
    {
		throw std::runtime_error("Failed to create instance!");
	}
	volkLoadInstance(instance);

#ifdef DEBUG
	if(vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create debug messenger!");
	}
#endif
}

void GraphicsDevice::CheckValidationLayerSupport()
{
	uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers)
        {
            if(strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if(!layerFound)
        {
            throw std::runtime_error("validation layer is not available");
        }
    }
}

void GraphicsDevice::SelectPhysicalDevice(VkSampleCountFlagBits requestedSampleCount)
{
	uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if(deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for(const auto& pd : devices)
    {
        if(IsDeviceSuitable(pd))
        {
            physicalDevice = pd;
			sampleCount = GetRequestedSampleCount(physicalDevice,requestedSampleCount);
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void GraphicsDevice::SetupDeviceFeatures()
{
	void* chainPtr = nullptr;
	features11 =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.shaderDrawParameters = VK_TRUE
	};

	descriptorIndexingFeatures =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.pNext = &features11,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE
	};
	chainPtr = &descriptorIndexingFeatures;

	deviceAddressEnabledFeatures =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
		.pNext = chainPtr,
		.bufferDeviceAddress = VK_TRUE
	};

	chainPtr = &deviceAddressEnabledFeatures;

	features = {};
	features.sampleRateShading = VK_TRUE;
	features.samplerAnisotropy = VK_TRUE;
	features.multiDrawIndirect = VK_TRUE;
	features.drawIndirectFirstInstance = VK_TRUE;
	features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	features.wideLines = VK_TRUE;

	features13 =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = chainPtr,
		.synchronization2 = true,
	};
	features2 =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13,
		.features = features,
	};
}

void GraphicsDevice::CreateLogicalDevice()
{
	SetupDeviceFeatures();

	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.GraphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;
	queueCreateInfos.push_back(queueCreateInfo);

	if(indices.GraphicsFamily != indices.PresentFamily)
	{
		queuePriority = 0;
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.PresentFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = nullptr;
	createInfo.pNext = &features2;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

#ifdef DEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();
#endif

	if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.GraphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.PresentFamily.value(), 0, &presentQueue);

	selectedFamilies = indices;
}

void GraphicsDevice::SetupCommandBuffers()
{
	QueueFamilyIndices indices = FindQueueFamilies();
	VK_CHECKF(CreateCommandPool(indices.GraphicsFamily.value(), &commandPool),"Failed to create command pool!")

	for(auto& frame : Frame)
	{
		VK_CHECKF(CreateCommandBuffer(commandPool, &frame.CmdBuffer),"Failed to allocate command buffers!")
	}
}

bool GraphicsDevice::IsDeviceSuitable(VkPhysicalDevice pd)
{
	QueueFamilyIndices indices = FindQueueFamilies(pd);

	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(pd, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for(const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	bool swapChainAdequate;
	if(requiredExtensions.empty())
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(pd);
		swapChainAdequate =!swapChainSupport.Formats.empty() &&!swapChainSupport.PresentModes.empty();
	}

	return indices.IsComplete() && requiredExtensions.empty() && swapChainAdequate;
}

VkSampleCountFlagBits
GraphicsDevice::GetRequestedSampleCount(VkPhysicalDevice pd, VkSampleCountFlagBits requestedSampleCount)
{
	VkSampleCountFlagBits maxSampleCount = VK_SAMPLE_COUNT_1_BIT;
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(pd, &deviceProperties);
	VkSampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;

	if(counts & VK_SAMPLE_COUNT_64_BIT) maxSampleCount = VK_SAMPLE_COUNT_64_BIT;
	else if(counts & VK_SAMPLE_COUNT_32_BIT) maxSampleCount = VK_SAMPLE_COUNT_32_BIT;
	else if(counts & VK_SAMPLE_COUNT_16_BIT) maxSampleCount = VK_SAMPLE_COUNT_16_BIT;
	else if(counts & VK_SAMPLE_COUNT_4_BIT) maxSampleCount = VK_SAMPLE_COUNT_4_BIT;
	else if(counts & VK_SAMPLE_COUNT_2_BIT) maxSampleCount = VK_SAMPLE_COUNT_2_BIT;
	else if(counts & VK_SAMPLE_COUNT_1_BIT) maxSampleCount = VK_SAMPLE_COUNT_1_BIT;

	return requestedSampleCount > maxSampleCount ? maxSampleCount : requestedSampleCount;
}