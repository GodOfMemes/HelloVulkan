#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include <volk.h> //<vulkan/vulkan.h> 
#include <vector>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class Buffer;
class Texture2D;

struct DescriptorWrite
{
	// pNext_ is for raytracing pipeline
	void* pNext_ = nullptr;
	VkDescriptorImageInfo* imageInfoPtr_ = nullptr;
	VkDescriptorBufferInfo* bufferInfoPtr_ = nullptr;
	// If you have an array of buffers/images, descriptorCount_ must be bigger than one
	uint32_t descriptorCount_ = 1u;
	VkDescriptorType descriptorType_;
	VkShaderStageFlags shaderStage_;
};

class DescriptorInfo
{
public:
	std::vector<DescriptorWrite> writes;
    
	void AddBuffer(
		const Buffer* buffer,
		VkDescriptorType dsType,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddImage(
		const Texture2D* image,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
	void UpdateBuffer(const Buffer* buffer, size_t bindingIndex);
	void UpdateImage(const Texture2D* image, size_t bindingIndex);

	// Descriptor indexing
	// TODO imageArrays is not stored so you need to supply it again if you update the descriptors
	void AddImageArray(
		const std::vector<VkDescriptorImageInfo>& imageArray,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);

private:
    std::unordered_map<size_t, VkDescriptorBufferInfo> _bufferMap;
	std::unordered_map<size_t, VkDescriptorImageInfo> _imageMap;

	// Descriptor indexing
	std::vector<VkDescriptorImageInfo> _imageArrays;

	void CheckBound(size_t bindingIndex) const;
};

class Descriptor
{
public:
    Descriptor(GraphicsDevice* gd)
        : _graphicsDevice(gd) {}
    ~Descriptor() { Destroy(); }

	void CreatePoolAndLayout(
		
		const DescriptorInfo& descriptorInfo,
		uint32_t frameCount,
		uint32_t setCountPerFrame,
		VkDescriptorPoolCreateFlags poolFlags = 0);

	void CreateSet(const DescriptorInfo& descriptorInfo, VkDescriptorSet* set);

	void AllocateSet(VkDescriptorSet* set);

	void UpdateSet(const DescriptorInfo& descriptorInfo, VkDescriptorSet* set);

	void Destroy();

    [[nodiscard]] VkDescriptorPool GetPool() { return _pool; }
    [[nodiscard]] VkDescriptorSetLayout GetLayout() { return _layout; }

private:
    GraphicsDevice* _graphicsDevice;
    VkDescriptorPool _pool = nullptr;
	VkDescriptorSetLayout _layout = nullptr;

	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t descriptorCount);
};