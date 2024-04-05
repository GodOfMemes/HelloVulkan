#include "ShaderSpecialization.hpp"

void ShaderSpecialization::ConsumeEntries(
	std::vector<VkSpecializationMapEntry>&& entries,
	void* data,
	size_t dataSize,
	VkShaderStageFlags shaderStages)
{
	_entries = std::move(entries);

	_specializationInfo.dataSize = dataSize;
	_specializationInfo.mapEntryCount = static_cast<uint32_t>(_entries.size());
	_specializationInfo.pMapEntries = _entries.data();
	_specializationInfo.pData = data;

	_shaderStages = shaderStages;
}

void ShaderSpecialization::Inject(std::vector<VkPipelineShaderStageCreateInfo>& shaderInfoArray)
{
	if (_entries.empty())
	{
		return;
	}

	for (auto& stageInfo : shaderInfoArray)
	{
		if (stageInfo.stage & _shaderStages)
		{
			stageInfo.pSpecializationInfo = &_specializationInfo;
			break;
		}
	}
}