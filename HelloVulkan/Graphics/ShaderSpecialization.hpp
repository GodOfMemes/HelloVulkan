#pragma once

#include <volk.h> //<vulkan/vulkan.h> 
#include <vector>

class ShaderSpecialization
{
public:
	ShaderSpecialization() = default;

    void ConsumeEntries(
		std::vector<VkSpecializationMapEntry>&& entries,
		void* data,
		size_t dataSize,
		VkShaderStageFlags shaderStages);
	void Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderInfoArray);
private:
	std::vector<VkSpecializationMapEntry> _entries = {};
	VkSpecializationInfo _specializationInfo = {};
	VkShaderStageFlags _shaderStages;
};