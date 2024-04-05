#pragma once

#include <volk.h> //<vulkan/vulkan.h> 

struct ImageBarrierInfo
{
	VkCommandBuffer commandBuffer;
	
	VkImageLayout oldLayout;
	VkPipelineStageFlags2 sourceStage;
	VkAccessFlags2 sourceAccess;
	
	VkImageLayout newLayout;
	VkPipelineStageFlags2 destinationStage;
	VkAccessFlags2 destinationAccess;
};

// TODO this abstraction is still fairly simple
class Barrier
{
public:
	static void CreateMemoryBarrier(VkCommandBuffer commandBuffer, const VkMemoryBarrier2* barriers, uint32_t barrierCount);
	static void CreateBufferBarrier(VkCommandBuffer commandBuffer, const VkBufferMemoryBarrier2* barriers, uint32_t barrierCount);
	static void CreateImageBarrier(const ImageBarrierInfo& info, const VkImageSubresourceRange& range, VkImage image);
	static void CreateImageBarrier(VkCommandBuffer commandBuffer, const VkImageMemoryBarrier2* barriers, uint32_t barrierCount);
};