#include "Texture2D.hpp"
#include "Barrier.hpp"
#include "Buffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Texture2D::Destroy()
{
	if (_defaultSampler)
	{
		vkDestroySampler(_graphicsDevice->GetDevice(), _defaultSampler, nullptr);
		_defaultSampler = nullptr;
	}

	if (_imageView)
	{
		vkDestroyImageView(_graphicsDevice->GetDevice(), _imageView, nullptr);
		_imageView = nullptr;
	}

	if (_image)
	{
		vmaDestroyImage(_graphicsDevice->GetAllocator(), _image, _allocation);
		_allocation = nullptr;
		_image = nullptr;
	}
}

void Texture2D::CreateImageResources(const std::string& filename)
{
	CreateFromFile(filename);
	GenerateMipmap(
		_mipCount,
		_size.width,
		_size.height,
		VK_IMAGE_LAYOUT_UNDEFINED);
	CreateImageView(
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		_mipCount,
		0u,
		_layerCount);
	CreateDefaultSampler(
		0.f, // minLod
		static_cast<float>(_mipCount)); // maxLod
}

void Texture2D::CreateImageResources(
	void* data,
	int width,
	int height)
{
	CreateImageFromData(
		data,
		width,
		height,
		MipMapCount(width, height),
		1u, // layerCount
		VK_FORMAT_R8G8B8A8_UNORM);
	GenerateMipmap(
		_mipCount,
		_size.width,
		_size.height,
		VK_IMAGE_LAYOUT_UNDEFINED);
	CreateImageView(
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		_mipCount,
		0u,
		_layerCount);
	CreateDefaultSampler(
		0.f, // minLod
		static_cast<float>(_mipCount)); // maxLod
}

void Texture2D::CreateFromFile(const std::string& filename)
{
	stbi_set_flip_vertically_on_load(false);

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("Failed to load image: " + filename);
	}

	CreateImageFromData(
		
		pixels,
		texWidth,
		texHeight,
		MipMapCount(texWidth, texHeight),
		1, // layerCount
		VK_FORMAT_R8G8B8A8_UNORM);

	stbi_image_free(pixels);
}

void Texture2D::CreateFromHDR(const std::string& filename)
{
	stbi_set_flip_vertically_on_load(true);
	int texWidth, texHeight, texChannels;
	float* pixels = stbi_loadf(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("Failed to load image: " + filename);
	}

	CreateImageFromData(
		pixels,
		texWidth,
		texHeight,
		MipMapCount(texWidth, texHeight),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT);
	stbi_image_free(pixels);
}

// TODO Rename to CreateColorAttachment
void Texture2D::CreateColorResources(
	uint32_t width, 
	uint32_t height,
	VkSampleCountFlagBits sampleCount)
{
	const VkFormat format = _graphicsDevice->GetImageFormat();
	CreateImage(
		
		width,
		height,
		1u, // mip
		1u, // layer
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		sampleCount);
	CreateImageView(
		
		format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);
	CreateDefaultSampler();
}

// TODO Rename to CreateDepthAttachment
void Texture2D::CreateDepthResources(
	  
	uint32_t width, 
	uint32_t height,
	uint32_t layerCount,
	VkSampleCountFlagBits sampleCount,
	VkImageUsageFlags additionalUsage)
{
	const VkFormat depthFormat = _graphicsDevice->GetDepthFormat();
	CreateImage(
		
		width,
		height,
		1u, // mip
		layerCount, // layer
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | additionalUsage,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		sampleCount);
	CreateImageView(
		
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);
	TransitionLayout(
		 
		depthFormat, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Texture2D::CreateImageFromData( 
	void* imageData,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t mipmapCount,
	uint32_t layerCount,
	VkFormat texFormat,
	VkImageCreateFlags flags)
{
	CreateImage(
		 
		texWidth, 
		texHeight, 
		mipmapCount,
		layerCount,
		texFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		flags);
	UpdateImage( texWidth, texHeight, texFormat, layerCount, imageData);
}

void Texture2D::CopyBufferToImage(
	 
	VkBuffer buffer,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount)
{
    _graphicsDevice->ImmediateSubmit([&](VkCommandBuffer commandBuffer)
    {
        const VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = VkImageSubresourceLayers {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = layerCount
            },
            .imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
            .imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
        };
        vkCmdCopyBufferToImage(commandBuffer, buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    });
}

void Texture2D::CreateImage(
	 
	uint32_t width,
	uint32_t height,
	uint32_t mipCount,
	uint32_t layerCount,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags imageUsage,
	VmaMemoryUsage memoryUsage,
	VkImageCreateFlags flags,
	VkSampleCountFlagBits sampleCount)
{
	_size.width = width;
	_size.height = height;
	_mipCount = mipCount;
	_layerCount = layerCount;
	_format = format;
	_msaaCount = sampleCount; // MSAA
	
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {.width = _size.width, .height = _size.height, .depth = 1 },
		.mipLevels = _mipCount,
		.arrayLayers = _layerCount,
		.samples = sampleCount,
		.tiling = tiling,
		.usage = imageUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocinfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	VK_CHECK(vmaCreateImage(
		_graphicsDevice->GetAllocator(),
		&imageInfo, 
		&allocinfo, 
		&_image, 
		&_allocation, 
		nullptr));
}

void Texture2D::CreateImageView(
	 
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkImageViewType viewType,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	CreateImageView(_graphicsDevice->GetDevice(),_image, _imageView, format, aspectFlags, viewType, mipLevel, mipCount, layerLevel, layerCount);
}

void Texture2D::CreateImageView(
	VkDevice device,
	VkImage image,
	VkImageView& view,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkImageViewType viewType,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.components =
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = mipLevel,
			.levelCount = mipCount,
			.baseArrayLayer = layerLevel,
			.layerCount = layerCount
		}
	};
	VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &view));
}

void Texture2D::CreateDefaultSampler(
	 
	float minLod,
	float maxLod,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	CreateSampler(
		
		_defaultSampler,
		minLod,
		maxLod,
		minFilter,
		maxFilter,
		addressMode
	);
}

void Texture2D::CreateSampler(
	 
	VkSampler& sampler,
	float minLod,
	float maxLod,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	const VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE, // Currently disabled
		.maxAnisotropy = 1.0f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = minLod,
		.maxLod = maxLod,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
	VK_CHECK(vkCreateSampler(_graphicsDevice->GetDevice(), &samplerInfo, nullptr, &sampler));
}

void Texture2D::UpdateImage(
	 
	uint32_t texWidth,
	uint32_t texHeight,
	VkFormat texFormat,
	uint32_t layerCount,
	const void* imageData,
	VkImageLayout sourceImageLayout)
{
	const uint32_t bytesPerPixel = BytesPerTexFormat(texFormat);

	const VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	const VkDeviceSize imageSize = layerSize * layerCount;

	Buffer stagingBuffer{_graphicsDevice};

	stagingBuffer.CreateBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY);

	stagingBuffer.UploadBufferData(imageData, imageSize);
	TransitionLayout( texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0u, 1u, 0u, layerCount);
	CopyBufferToImage( stagingBuffer.GetHandle(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
	TransitionLayout( texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0u, 1u, 0u, layerCount);

	stagingBuffer.Destroy();
}

void Texture2D::TransitionLayout(
	 
	VkImageLayout oldLayout,
	VkImageLayout newLayout)
{
	TransitionLayout( _format, oldLayout, newLayout, 0u, _mipCount, 0u, _layerCount);
}

void Texture2D::TransitionLayout(
	 
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
    _graphicsDevice->ImmediateSubmit([&](VkCommandBuffer commandBuffer)
    {
        TransitionLayoutCommand(commandBuffer, 
                _image, 
                format, 
                oldLayout, 
                newLayout, 
                mipLevel, 
                mipCount,
                layerLevel,
                layerCount);
    });
}

void Texture2D::TransitionLayoutCommand(
	VkCommandBuffer commandBuffer,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, // Default
		.srcAccessMask = 0, // Set to the correct value
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, // Default
		.dstAccessMask = 0, // Set to the correct value
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = mipLevel,
			.levelCount = mipCount,
			.baseArrayLayer = layerLevel,
			.layerCount = layerCount
		}
	};

	// If depth
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		// If stencil
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// We have to set the access masks and stages
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT; // This is not VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	}
	// Convert depth texture from undefined state to depth-stencil buffer
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
	}
	// Swapchain
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// Convert back from read-only to updateable
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	}
	// Wait for render pass to complete
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0; 
		barrier.dstAccessMask = 0;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	// Convert back from read-only to color attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// Convert back from read-only to depth attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	}
	// Convert from updateable texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	// Swapchain
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	}
	// Convert from updateable texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	// Convert from updateable depth texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}

	Barrier::CreateImageBarrier(commandBuffer, &barrier, 1u);
}

// TODO This function uses CreateBarrier() instead on TransitionLayout()
void Texture2D::GenerateMipmap(
	 
	uint32_t maxMipLevels,
	uint32_t width,
	uint32_t height,
	VkImageLayout currentImageLayout
)
{
    _graphicsDevice->ImmediateSubmit([&](VkCommandBuffer commandBuffer)
    {
        Barrier::CreateImageBarrier(
            {
                .commandBuffer = commandBuffer,
                .oldLayout = currentImageLayout,
                .sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .sourceAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .destinationStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .destinationAccess = VK_ACCESS_2_TRANSFER_READ_BIT
            },
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u, // The number of mipmap levels (starting from baseMipLevel) accessible to the view
                .baseArrayLayer = 0,
                .layerCount = _layerCount
            },
            _image
            );

        for (uint32_t i = 1; i < maxMipLevels; ++i)
        {
            // TODO use VkImageBlit2
            VkImageBlit imageBlit{};

            // Source
            imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.srcSubresource.layerCount = _layerCount;
            imageBlit.srcSubresource.mipLevel = i - 1;
            imageBlit.srcOffsets[1].x = static_cast<int32_t>(width >> (i - 1));
            imageBlit.srcOffsets[1].y = static_cast<int32_t>(height >> (i - 1));
            imageBlit.srcOffsets[1].z = 1;

            // Destination
            imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBlit.dstSubresource.layerCount = _layerCount;
            imageBlit.dstSubresource.mipLevel = i;
            imageBlit.dstOffsets[1].x = static_cast<int32_t>(width >> i);
            imageBlit.dstOffsets[1].y = static_cast<int32_t>(height >> i);
            imageBlit.dstOffsets[1].z = 1;

            VkImageSubresourceRange subRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = i,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = _layerCount
            };

            // Transition current mip level to transfer dest
            Barrier::CreateImageBarrier(
                {
                    .commandBuffer = commandBuffer,
                    .oldLayout = currentImageLayout,
                    .sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .sourceAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .destinationStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .destinationAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT
                },
                subRange,
                _image);

            vkCmdBlitImage(
                commandBuffer,
                _image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                _image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &imageBlit,
                VK_FILTER_LINEAR);

            // Transition back
            Barrier::CreateImageBarrier(
                {
                    .commandBuffer = commandBuffer,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .sourceStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .sourceAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .destinationStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .destinationAccess = VK_ACCESS_2_TRANSFER_READ_BIT
                },
                subRange,
                _image);
        }

        // Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        Barrier::CreateImageBarrier({ 
            .commandBuffer = commandBuffer,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .sourceAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .destinationStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .destinationAccess = VK_ACCESS_2_SHADER_READ_BIT
        },
        {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = _mipCount,
            .baseArrayLayer = 0,
            .layerCount = _layerCount
        },
        _image
        );
    });
}

uint32_t Texture2D::BytesPerTexFormat(VkFormat fmt)
{
	switch (fmt)
	{
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_UNORM:
		return 1;
	case VK_FORMAT_R16_SFLOAT:
		return 2;
	case VK_FORMAT_R16G16_SFLOAT:
	case VK_FORMAT_R16G16_SNORM:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case VK_FORMAT_R32G32_SFLOAT:
		return 2 * sizeof(float);
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * sizeof(uint16_t);
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * sizeof(float);
	default:
		break;
	}
	return 0;
}

VkDescriptorImageInfo Texture2D::GetDescriptorImageInfo() const
{
	return
	{
		_defaultSampler,
		_imageView,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
}