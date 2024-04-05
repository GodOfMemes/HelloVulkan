#pragma once

#include <Graphics/Resource.hpp>
#include <Graphics/Buffer.hpp>
#include "Graphics/GraphicsDevice.hpp"
#include <array>

struct ClusterForwardResource : ResourceBase
{
public:
	ClusterForwardResource(GraphicsDevice* gd) : ResourceBase(gd) {}
	~ClusterForwardResource()
	{
		Destroy();
	}

	void Destroy() override;
	void CreateBuffers(uint32_t lightCount);
	
public:
	bool aabbDirty_;
	Buffer aabbBuffer_{gd};
	Buffer lightCellsBuffer_{gd};
	Buffer lightIndicesBuffer_{gd};

	// Use frame-in-flight because it is reset by the CPU
	std::array<Buffer, MAX_FRAMES_IN_FLIGHT> globalIndexCountBuffers_ = {gd,gd}; // Atomic counter
};