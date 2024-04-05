#include "Barrier.hpp"

void Barrier::CreateMemoryBarrier(
	VkCommandBuffer commandBuffer,
	const VkMemoryBarrier2* barriers,
	uint32_t barrierCount
)
{
	const VkDependencyInfo dependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = barrierCount,
		.pMemoryBarriers = barriers
	};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void Barrier::CreateBufferBarrier(
	VkCommandBuffer commandBuffer,
	const VkBufferMemoryBarrier2* barriers,
	uint32_t barrierCount
)
{
	const VkDependencyInfo dependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.bufferMemoryBarrierCount = barrierCount,
		.pBufferMemoryBarriers = barriers
	};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void Barrier::CreateImageBarrier(const ImageBarrierInfo& info, const VkImageSubresourceRange& range, VkImage image)
{
	const VkImageMemoryBarrier2 barrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = info.sourceStage,
		.srcAccessMask = info.sourceAccess,
		.dstStageMask = info.destinationStage,
		.dstAccessMask = info.destinationAccess,
		.oldLayout = info.oldLayout,
		.newLayout = info.newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = range
	};
	CreateImageBarrier(info.commandBuffer, &barrier, 1u);
}

void Barrier::CreateImageBarrier(
	VkCommandBuffer commandBuffer,
	const VkImageMemoryBarrier2* barriers,
	uint32_t barrierCount
)
{
	const VkDependencyInfo dependencyInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = nullptr ,
		.imageMemoryBarrierCount = barrierCount,
		.pImageMemoryBarriers = barriers
	};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}