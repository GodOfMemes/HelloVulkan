#include "Descriptor.hpp"
#include "Texture2D.hpp"
#include "Buffer.hpp"

void DescriptorInfo::AddBuffer(
	const Buffer* buffer,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorBufferInfo* bufferInfo = nullptr;
	if (buffer)
	{
		const size_t index = writes.size();
		_bufferMap[index] = buffer->GetBufferInfo();
		bufferInfo = &(_bufferMap[index]);
	}

	writes.push_back
	({
		.bufferInfoPtr_ = bufferInfo,
		.descriptorCount_ = 1u,
		.descriptorType_ = dsType,
		.shaderStage_ = stageFlags
		});
}

void DescriptorInfo::AddImage(
	const Texture2D* image,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		const size_t index = writes.size();
		_imageMap[index] = image->GetDescriptorImageInfo();
		imageInfo = &(_imageMap[index]);
	}

	writes.push_back
	({
		.imageInfoPtr_ = imageInfo,
		.descriptorCount_ = 1u,
		.descriptorType_ = dsType,
		.shaderStage_ = stageFlags
		});
}

// Special case for descriptor indexing
void DescriptorInfo::AddImageArray(
	const std::vector<VkDescriptorImageInfo>& imageArray,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags)
{
	_imageArrays = imageArray;

	writes.push_back
	({
		.imageInfoPtr_ = _imageArrays.data(),
		.descriptorCount_ = static_cast<uint32_t>(_imageArrays.size()),
		.descriptorType_ = dsType,
		.shaderStage_ = stageFlags
		});
}

void DescriptorInfo::UpdateBuffer(const Buffer* buffer, size_t bindingIndex)
{
	CheckBound(bindingIndex);

	VkDescriptorBufferInfo* bufferInfo = nullptr;
	if (buffer)
	{
		_bufferMap[bindingIndex] = buffer->GetBufferInfo();
		bufferInfo = &(_bufferMap[bindingIndex]);
	}

	writes[bindingIndex].bufferInfoPtr_ = bufferInfo;
}

void DescriptorInfo::UpdateImage(const Texture2D* image, size_t bindingIndex)
{
	CheckBound(bindingIndex);

	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		_imageMap[bindingIndex] = image->GetDescriptorImageInfo();
		imageInfo = &(_imageMap[bindingIndex]);
	}

	writes[bindingIndex].imageInfoPtr_ = imageInfo;
}

void DescriptorInfo::CheckBound(size_t bindingIndex) const
{
	if (bindingIndex < 0 || bindingIndex >= writes.size())
	{
		std::cerr << "Invalid bindingIndex\n";
	}
}

void Descriptor::CreatePoolAndLayout(
	const DescriptorInfo& descriptorInfo,
	uint32_t frameCount,
	uint32_t setCountPerFrame,
	VkDescriptorPoolCreateFlags poolFlags)
{

	uint32_t uboCount = 0;
	uint32_t ssboCount = 0;
	uint32_t samplerCount = 0;
	uint32_t storageImageCount = 0;
	uint32_t accelerationStructureCount = 0;

	// Create pool
	for (auto& write : descriptorInfo.writes)
	{
		if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			uboCount += write.descriptorCount_;
		}
		else if(write.descriptorType_ == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			ssboCount += write.descriptorCount_;
		}
		else if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			samplerCount += write.descriptorCount_;
		}
		else if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
		{
			accelerationStructureCount += write.descriptorCount_;
		}
		else if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			storageImageCount += write.descriptorCount_;
		}
		else
		{
			std::cerr << "Descriptor type is currently not supported\n";
		}
	}

	// Frame-in-flight
	uboCount *= frameCount;
	ssboCount *= frameCount;
	samplerCount *= frameCount;
	storageImageCount *= frameCount;
	accelerationStructureCount *= frameCount;

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (uboCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = uboCount
			});
	}

	if (ssboCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = ssboCount
			});
	}

	if (samplerCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = samplerCount
			});
	}

	if (storageImageCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = storageImageCount
			});
	}

	if (accelerationStructureCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
				.descriptorCount = accelerationStructureCount
			});
	}

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = poolFlags,
		.maxSets = static_cast<uint32_t>(frameCount * setCountPerFrame),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(_graphicsDevice->GetDevice(), &poolInfo, nullptr, &_pool));
	
	// Create Layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	uint32_t bindingIndex = 0;
	for (auto& write : descriptorInfo.writes)
	{
		layoutBindings.emplace_back(
			CreateDescriptorSetLayoutBinding
			(
				bindingIndex++,
				write.descriptorType_,
				write.shaderStage_,
				write.descriptorCount_
			)
		);
	}
	const VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
		.pBindings = layoutBindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(_graphicsDevice->GetDevice(), &layoutInfo, nullptr, &_layout));
}

void Descriptor::CreateSet(
	const DescriptorInfo& descriptorInfo,
	VkDescriptorSet* set)
{
	AllocateSet(set);

	UpdateSet(descriptorInfo, set);
}

void Descriptor::AllocateSet(VkDescriptorSet* set)
{
	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = _pool,
		.descriptorSetCount = 1u,
		.pSetLayouts = &_layout
	};

	VK_CHECK(vkAllocateDescriptorSets(_graphicsDevice->GetDevice(), &allocInfo, set));
}

void Descriptor::UpdateSet(
	
	const DescriptorInfo& descriptorInfo,
	VkDescriptorSet* set)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	uint32_t bindIndex = 0;

	descriptorWrites.reserve(descriptorInfo.writes.size());
	for (auto& write : descriptorInfo.writes)
	{
		descriptorWrites.push_back({
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = write.pNext_,
			.dstSet = *set, // Dereference
			.dstBinding = bindIndex++,
			.dstArrayElement = 0,
			.descriptorCount = write.descriptorCount_,
			.descriptorType = write.descriptorType_,
			.pImageInfo = write.imageInfoPtr_,
			.pBufferInfo = write.bufferInfoPtr_,
			.pTexelBufferView = nullptr
		});
	}

	vkUpdateDescriptorSets
	(
		_graphicsDevice->GetDevice(),
		static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(),
		0,
		nullptr
	);
}

void Descriptor::Destroy()
{
	if (_layout)
	{
		vkDestroyDescriptorSetLayout(_graphicsDevice->GetDevice(), _layout, nullptr);
		_layout = nullptr;
	}

	if (_pool)
	{
		vkDestroyDescriptorPool(_graphicsDevice->GetDevice(), _pool, nullptr);
		_pool = nullptr;
	}
}

VkDescriptorSetLayoutBinding Descriptor::CreateDescriptorSetLayoutBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t descriptorCount)
{
	return VkDescriptorSetLayoutBinding
	{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}