#ifndef RESOURCES_CLUSTER_FORWARD
#define RESOURCES_CLUSTER_FORWARD

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "Configs.h"

#include <array>

enum class AABBFlag : uint8_t
{
	Clean = 0u,
	Dirty = 1u
};

struct ResourcesClusterForward
{
public:
	ResourcesClusterForward() = default;
	~ResourcesClusterForward()
	{
		Destroy();
	}

	void Destroy();
	void CreateBuffers(VulkanContext& ctx, uint32_t lightCount);
	void SetAABBDirty();
	bool IsAABBDirty(uint32_t frameIndex) { return aabbDirtyFlags_[frameIndex] == AABBFlag::Dirty; }
	void SetAABBClean(uint32_t frameIndex) { aabbDirtyFlags_[frameIndex] = AABBFlag::Clean; }
	
public:
	std::array<AABBFlag, AppConfig::FrameOverlapCount> aabbDirtyFlags_; // For window resizing
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> aabbBuffers_;
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> globalIndexCountBuffers_; // Atomic counter
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> lightCellsBuffers_;
	std::array<VulkanBuffer, AppConfig::FrameOverlapCount> lightIndicesBuffers_;
};

#endif