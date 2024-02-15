#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "PipelineBase.h"

class PipelineImGui final : public PipelineBase
{
public:
	PipelineImGui(
		VulkanDevice& vkDev, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~PipelineImGui();

	void StartImGui();
	void EndImGui();
	void DrawEmptyImGui();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;
};

#endif