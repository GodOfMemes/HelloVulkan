#pragma once

#include "GraphicsDevice.hpp"
#include "RenderPass.hpp"
#include "ShaderSpecialization.hpp"
#include "Buffer.hpp"
#include "Descriptor.hpp"
#include "Framebuffer.hpp"
#include "Mesh/MeshVertex.hpp"
#include <vulkan/vulkan_core.h>

enum class PipelineType : uint8_t
{
	GraphicsOnScreen = 0u,
	GraphicsOffScreen = 1u,
	Compute = 2u,
};

struct PipelineConfig
{
	PipelineType type_ = PipelineType::GraphicsOnScreen;
	VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;
	VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	uint32_t PatchControlPointsCount_ = 0;
	bool vertexBufferBind_ = false;
	bool depthTest_ = true;
	bool depthWrite_ = true;
	bool useBlending_ = true;

	bool customViewportSize_ = false;
	VkExtent2D viewportSize = {0,0};
	float lineWidth_ = 1.0f;
};

// Default pipeline create info
struct PipelineCreateInfo
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlending;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineDynamicStateCreateInfo dynamicState;
	VkPipelineTessellationStateCreateInfo tessellationState;

	PipelineCreateInfo() :
		vertexInputInfo({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr
		}),
		inputAssembly({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		}),
		viewport({
			.x = 0.f,
			.y = 0.f,
			// Below don't matter because of dynamic state
			.width = 0.f,
			.height = 0.f,
			.minDepth = 0.f,
			.maxDepth = 1.f
		}),
		scissor({
			.offset = { 0, 0 },
			// Below don't matter because of dynamic state
			.extent = { 0, 0}
		}),
		viewportState({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		}),
		rasterizer({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 1.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 1.f,
			.lineWidth = 1.f
		}),
		multisampling({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0u,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0.f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		}),
		colorBlendAttachment({
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		}),
		colorBlending({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
		}),
		depthStencil({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL, // Needed for skybox rendering
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
		}),
		// Change below if you want window resizing
		dynamicState({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = 0u,
			.pDynamicStates = nullptr
		}),
		tessellationState({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.patchControlPoints = 0
		})
	{
	}
};

/*
This mainly encapsulates a graphics pipeline, framebuffers, and a render pass.
A pipeline can be either
	* Offscreen graphics (draw to an image)
	* Onscreen graphics (draw to a swapchain image)
	* Compute
 */
class PipelineBase
{
    friend class GraphicsDevice;
public:
	explicit PipelineBase(
		GraphicsDevice* ctx,
		const PipelineConfig& config);
	virtual ~PipelineBase();

	// TODO Maybe rename to RecordCommandBuffer
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer) {}

	virtual void SetCameraUBO(CameraUBO& ubo)
	{
        if(cameraUBOBuffers.empty()) return;
		const uint32_t frameIndex = gd->GetCurrentFrameIdx();
		cameraUBOBuffers[frameIndex].UploadBufferData(&ubo, sizeof(CameraUBO));
	}

protected:
	GraphicsDevice* gd;
	PipelineConfig config;

	// Keep this as a vector because it can be empty when not used
	std::vector<Buffer> cameraUBOBuffers;

	Framebuffer framebuffer;
	Descriptor descriptor;
	RenderPass renderPass;
	ShaderSpecialization specializationConstants;
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline pipeline = nullptr;

    // If the window is resized
	virtual void OnWindowResized();
    
	bool IsOffscreen() const
	{
		return config.type_ == PipelineType::GraphicsOffScreen;
	}

	void BindPipeline(VkCommandBuffer commandBuffer);

	void CreatePipelineLayout(
		VkDescriptorSetLayout dsLayout,
		VkPipelineLayout* pipelineLayout,
		uint32_t pushConstantSize = 0,
		VkShaderStageFlags pushConstantShaderStage = 0);

	void CreateGraphicsPipeline(
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<std::string>& shaderFiles,
		VkPipeline* pipeline,
        const std::vector<VkVertexInputBindingDescription>& bindingDescriptions = DefaultVertexData::GetBindingDescription(),
	    const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = DefaultVertexData::GetAttributeDescriptions());

	void CreateComputePipeline(const std::string& shaderFile);
};