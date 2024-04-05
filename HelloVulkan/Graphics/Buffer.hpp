#pragma once

#include "GraphicsDevice.hpp"

class Buffer
{
public:
    Buffer(GraphicsDevice* graphicsDevice) 
        : _graphicsDevice(graphicsDevice) {}

    ~Buffer() = default;

    void Destroy();

    void CreateBuffer(
        VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateGPUOnlyIndirectBuffer(const void* bufferData,VkDeviceSize size);

	void CreateMappedIndirectBuffer(VkDeviceSize size);

	VkDrawIndirectCommand* MapIndirectBuffer();
	void UnmapIndirectBuffer();

	void CreateBufferWithShaderDeviceAddress(
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateGPUOnlyBuffer(
		VkDeviceSize bufferSize_,
		const void* bufferData,
		VkBufferUsageFlags bufferUsage
	);

	void CopyFrom(VkBuffer srcBuffer, VkDeviceSize size);

	void UploadOffsetBufferData(const void* data, VkDeviceSize offset, VkDeviceSize dataSize);

	void UploadBufferData(const void* data,const size_t dataSize);

	void DownloadBufferData(void* outData, const size_t dataSize);

	VkDescriptorBufferInfo GetBufferInfo() const
	{
		return
		{
			.buffer = _handle,
			.offset = 0,
			.range = _size
		};
	}

	void SetDebugName(const std::string& debugName)
	{
		if(_handle)
		{
			_graphicsDevice->SetVkObjectName(_handle, VK_OBJECT_TYPE_BUFFER, debugName.c_str());
		}
	}

	// Helper function
	static void CreateMultipleUniformBuffers(
		GraphicsDevice* gd,
		std::vector<Buffer>& buffers,
		uint32_t dataSize,
		size_t bufferCount);

    [[nodiscard]] VkBuffer GetHandle() const { return _handle; }
    [[nodiscard]] VkDeviceSize GetSize() const { return _size; }
	[[nodiscard]] uint64_t GetAddress() const { return _deviceAddress; }
private:
    GraphicsDevice* _graphicsDevice;
    VkBuffer _handle = nullptr;
    VmaAllocation _allocation = nullptr;
    VmaAllocationInfo _allocationInfo;
    VkDeviceSize _size;
    uint64_t _deviceAddress = 0;
};