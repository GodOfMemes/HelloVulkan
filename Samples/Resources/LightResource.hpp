#pragma once

#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Resource.hpp>
#include <Graphics/Buffer.hpp>
#include "UIData.hpp"

// A single light
struct LightData
{
	alignas(16)
	glm::vec4 position_;
	alignas(16)
	glm::vec4 color_ = glm::vec4(1.0f);
	alignas(4)
	float radius_ = 1.0f;
};

// Clustered forward
struct AABB
{
	alignas(16)
	glm::vec4 minPoint;
	alignas(16)
	glm::vec4 maxPoint;
};

// Clustered forward
struct LightCell
{
	alignas(4)
	uint32_t offset;
	alignas(4)
	uint32_t count;
};

// A collection of lights, including SSBO
struct LightResource : ResourceBase
{
public:
	LightResource(GraphicsDevice* gd) : ResourceBase(gd) {}
	~LightResource()
	{
		Destroy();
	}

	void Destroy() override;
	void AddLights(const std::vector<LightData>& lights);
	
	void UpdateLightPosition(size_t index, const std::span<float> position);

	Buffer* GetVulkanBufferPtr() { return &storageBuffer_;  }
	uint32_t GetLightCount() const { return lightCount_; }
	
	VkDescriptorBufferInfo GetBufferInfo() const
	{
		return storageBuffer_.GetBufferInfo();
	}

public:
	std::vector<LightData> lights_ = {};

private:
	uint32_t lightCount_ = 0;
	Buffer storageBuffer_{gd};
};