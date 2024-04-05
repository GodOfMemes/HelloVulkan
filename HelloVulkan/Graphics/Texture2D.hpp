#pragma once

#include "GraphicsDevice.hpp"
#include <vulkan/vulkan_core.h>

class Texture2D
{
public:
    Texture2D(GraphicsDevice* graphicsDevice) 
        : _graphicsDevice(graphicsDevice) {}

    ~Texture2D() = default;

    void Destroy();

    void CreateFromFile(const std::string& filename);

	// Create a mipmapped image, an image view, and a sampler
	void CreateImageResources(const std::string& filename);

	// Create a mipmapped image, an image view, and a sampler
	void CreateImageResources(
        void* data,
		int width,
		int height);

	void CreateFromHDR(const std::string& filename);

	void CreateSampler(
        VkSampler& sampler,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateDefaultSampler(
        float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateImage(
        uint32_t width,
		uint32_t height,
		uint32_t mipCount,
		uint32_t layerCount,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags imageUsage,
		VmaMemoryUsage memoryUsage,
		VkImageCreateFlags flags = 0,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	void CreateImageFromData(
        void* imageData,
		uint32_t texWidth,
		uint32_t texHeight,
		uint32_t mipmapCount,
		uint32_t layerCount,
		VkFormat texFormat,
		VkImageCreateFlags flags = 0);

	// This is used for offscreen rendering as a color attachment
	void CreateColorResources(
        uint32_t width, 
		uint32_t height,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	// Depth attachment for onscreen/offscreen rendering
	void CreateDepthResources(
        uint32_t width, 
		uint32_t height,
		uint32_t layerCount,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
		VkImageUsageFlags additionalUsage = 0);

	void GenerateMipmap(
        uint32_t maxMipLevels,
		uint32_t width,
		uint32_t height,
		VkImageLayout currentImageLayout);

	void CreateImageView(
        VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u);

	static void CreateImageView(
        VkDevice device,
        VkImage image,
		VkImageView& view,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u);


	void CopyBufferToImage(
        VkBuffer buffer,
		uint32_t width,
		uint32_t height,
		uint32_t layerCount = 1);

	// This transitions all mip levels and all layers
	void TransitionLayout(
        VkImageLayout oldLayout,
		VkImageLayout newLayout);
	
	void TransitionLayout(
        VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		// By default, this transitions one mip level and one layers
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u
	);

	static void TransitionLayoutCommand(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		// By default, this transitions one mip level and one layers
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u
	);

	void SetDebugName(const std::string& debugName)
	{
		if(_image)
		{
			_graphicsDevice->SetVkObjectName(_image, VK_OBJECT_TYPE_IMAGE, debugName.c_str());
		}
	}

	// To create descriptor sets
	[[nodiscard]] VkDescriptorImageInfo GetDescriptorImageInfo() const;

    [[nodiscard]] VkImage GetHandle() const { return _image; }
    [[nodiscard]] VkImageView GetView() const { return _imageView; }
    [[nodiscard]] VkSampler GetSampler() const { return _defaultSampler; }
    [[nodiscard]] VkExtent2D GetSize() const { return _size; }
    [[nodiscard]] uint32_t GetMipCount() const { return _mipCount; }
    [[nodiscard]] uint32_t GetLayerCount() const { return _layerCount; }
    [[nodiscard]] VkFormat GetFormat() const { return _format; }
	[[nodiscard]] VkSampleCountFlagBits GetSampleCount() const { return _msaaCount; }
private:
    GraphicsDevice* _graphicsDevice = nullptr;
    VkImage _image = nullptr;
    VkImageView _imageView = nullptr;
    VmaAllocation _allocation = nullptr;
    VkSampler _defaultSampler = nullptr;
    VkExtent2D _size{0,0};
    uint32_t _mipCount = 0;
    uint32_t _layerCount = 0;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits _msaaCount = VK_SAMPLE_COUNT_1_BIT;

    void UpdateImage(
		uint32_t texWidth,
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount,
		const void* imageData,
		VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

	uint32_t BytesPerTexFormat(VkFormat fmt);

    int MipMapCount(int w, int h)
	{
		int levels = 1;
		while ((w | h) >> levels)
		{
			levels += 1;
		}
		return levels;
	}

	int MipMapCount(int size)
	{
		int levels = 1;
		while (size >> levels)
		{
			levels += 1;
		}
		return levels;
	}
};