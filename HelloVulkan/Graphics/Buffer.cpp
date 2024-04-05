#include "Buffer.hpp"

void Buffer::CreateBuffer(
	
	VkDeviceSize size,
	VkBufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	VmaAllocationCreateFlags flags)
{
	_size = size;
	
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VmaAllocationCreateInfo vmaAllocInfo = {
		.flags = flags,
		.usage = memoryUsage,
	};

	VK_CHECK(vmaCreateBuffer(
		_graphicsDevice->GetAllocator(),
		&bufferInfo, 
		&vmaAllocInfo, 
		&_handle, 
		&_allocation,
		&_allocationInfo));

	if (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = _handle
		};

		_deviceAddress = vkGetBufferDeviceAddressKHR(_graphicsDevice->GetDevice(), &bufferDeviceAddressInfo);
	}
}

void Buffer::Destroy()
{
	if (_handle)
	{
		vmaDestroyBuffer(_graphicsDevice->GetAllocator(), _handle, _allocation);
		_handle = nullptr;
		_allocation = nullptr;
	}
}

void Buffer::CreateGPUOnlyIndirectBuffer(
		
		const void* bufferData,
		VkDeviceSize size)
{
	CreateGPUOnlyBuffer(
		size,
		bufferData,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
}

void Buffer::CreateMappedIndirectBuffer(
	
	VkDeviceSize size)
{
	CreateBuffer(
		size,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // TODO Is it possible to be GPU only?
		VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

VkDrawIndirectCommand* Buffer::MapIndirectBuffer()
{
	VkDrawIndirectCommand* mappedData = nullptr;
	vmaMapMemory(_graphicsDevice->GetAllocator(), _allocation, (void**)&mappedData);
	return mappedData;
}

void Buffer::UnmapIndirectBuffer()
{
	vmaUnmapMemory(_graphicsDevice->GetAllocator(), _allocation);
}

void Buffer::CreateBufferWithShaderDeviceAddress(
	VkDeviceSize size,
	VkBufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	VmaAllocationCreateFlags flags)
{
	CreateBuffer(
		size,
		bufferUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		memoryUsage,
		flags);
}

void Buffer::CreateGPUOnlyBuffer(
	VkDeviceSize bufferSize_,
	const void* bufferData,
	VkBufferUsageFlags bufferUsage)
{
	Buffer stagingBuffer{_graphicsDevice};
	stagingBuffer.CreateBuffer(
		
		bufferSize_,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY // TODO Deprecated flag
	);

	void* data;
	vmaMapMemory(stagingBuffer._graphicsDevice->GetAllocator(), stagingBuffer._allocation,  &data);
	memcpy(data, bufferData, bufferSize_);
	vmaUnmapMemory(stagingBuffer._graphicsDevice->GetAllocator(), stagingBuffer._allocation);

	CreateBuffer(
		
		bufferSize_,
		bufferUsage,
		VMA_MEMORY_USAGE_GPU_ONLY, // TODO Deprecated flag
		0);
	CopyFrom( stagingBuffer._handle, bufferSize_);

	stagingBuffer.Destroy();
}

void Buffer::CopyFrom( VkBuffer srcBuffer, VkDeviceSize size)
{
    _graphicsDevice->ImmediateSubmit([&](VkCommandBuffer commandBuffer)
    {
        const VkBufferCopy copyRegion = {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = size
            };

        vkCmdCopyBuffer(commandBuffer, srcBuffer, _handle, 1, &copyRegion);
    });
}

void Buffer::UploadOffsetBufferData(
	
	const void* data,
	VkDeviceSize offset,
	VkDeviceSize dataSize)
{
	vmaCopyMemoryToAllocation(_graphicsDevice->GetAllocator(), data, _allocation, offset, dataSize);
}

void Buffer::UploadBufferData(
	
	const void* data,
	const size_t dataSize)
{
	vmaCopyMemoryToAllocation(_graphicsDevice->GetAllocator(), data, _allocation, 0, dataSize);
}

void Buffer::DownloadBufferData(
	void* outData,
	const size_t dataSize)
{
	void* mappedData = nullptr;
	vmaMapMemory(_graphicsDevice->GetAllocator(), _allocation, &mappedData);
	memcpy(outData, mappedData, dataSize);
	vmaUnmapMemory(_graphicsDevice->GetAllocator(), _allocation);
}

void Buffer::CreateMultipleUniformBuffers(
    GraphicsDevice* gd,
	std::vector<Buffer>& buffers,
	uint32_t dataSize,
	size_t bufferCount)
{
	buffers.resize(bufferCount,{gd});
	for (size_t i = 0; i < bufferCount; i++)
	{
		buffers[i].CreateBuffer(
			dataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
	}
}