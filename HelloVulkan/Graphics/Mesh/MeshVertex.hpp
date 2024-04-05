#pragma once

#include <volk.h> //<vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <vector>

struct DefaultVertexData
{
	glm::vec3 position;
	float uvX;
	glm::vec3 normal;
	float uvY;
	glm::vec4 color;

    static std::vector<VkVertexInputBindingDescription> GetBindingDescription()
	{
		return
		{{
			.binding = 0,
			.stride = sizeof(DefaultVertexData),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		}};
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back(
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(DefaultVertexData, position)
		});
		attributeDescriptions.push_back(
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = offsetof(DefaultVertexData, uvX)
		});
		attributeDescriptions.push_back(
		{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(DefaultVertexData, normal)
		});
		attributeDescriptions.push_back(
		{
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32_SFLOAT,
			.offset = offsetof(DefaultVertexData, uvY)
		});
		attributeDescriptions.push_back(
		{
			.location = 4,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
			.offset = offsetof(DefaultVertexData, color)
		});

		return attributeDescriptions;
	}
};