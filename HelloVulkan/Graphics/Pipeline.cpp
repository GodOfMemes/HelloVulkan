#include "Pipeline.hpp"
#include "SwapChain.hpp"
#include "Shader.hpp"

// Constructor
PipelineBase::PipelineBase(
	GraphicsDevice* ctx,
	const PipelineConfig& config) :
	gd(ctx),
	config(config),
    framebuffer(gd),
    descriptor(gd),
    renderPass(gd)
{
}

// Destructor
PipelineBase::~PipelineBase()
{
	for (auto uboBuffer : cameraUBOBuffers)
	{
		uboBuffer.Destroy();
	}

	framebuffer.Destroy();
	descriptor.Destroy();
	renderPass.Destroy();

	vkDestroyPipelineLayout(gd->GetDevice(), pipelineLayout, nullptr);
	vkDestroyPipeline(gd->GetDevice(), pipeline, nullptr);
}

void PipelineBase::BindPipeline(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    auto swapExtent = gd->GetSwapChain()->GetExtent();
	if (config.customViewportSize_)
	{
		swapExtent = config.viewportSize;
	}
	const VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(swapExtent.width),
		.height = static_cast<float>(swapExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void PipelineBase::OnWindowResized()
{
	// If this is compute pipeline, no need to recreate framebuffer
	if (config.type_ == PipelineType::Compute)
	{
		return;
	}

	framebuffer.Destroy();
	framebuffer.Recreate();
}

void PipelineBase::CreatePipelineLayout(
	VkDescriptorSetLayout dsLayout,
	VkPipelineLayout* pipelineLayout,
	uint32_t pushConstantSize,
	VkShaderStageFlags pushConstantShaderStage)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	VkPushConstantRange pcRange;
	if (pushConstantSize > 0)
	{
		pcRange =
		{
			.stageFlags = pushConstantShaderStage,
			.offset = 0u,
			.size = pushConstantSize,
		};
		pipelineLayoutInfo.pPushConstantRanges = &pcRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1u;
	}

	VK_CHECK(vkCreatePipelineLayout(gd->GetDevice(), &pipelineLayoutInfo, nullptr, pipelineLayout));
}

void PipelineBase::CreateGraphicsPipeline(
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline,
    const std::vector<VkVertexInputBindingDescription>& bindingDescriptions,
	const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
{
	std::vector<Shader> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(shaderFiles.size());
	shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(gd->GetDevice(), file));
		const VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Add specialization constants if any
	specializationConstants.Inject(shaderStages);

	// Pipeline create info
	PipelineCreateInfo pInfo{};

	if (config.vertexBufferBind_)
	{
		pInfo.vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
		pInfo.vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		pInfo.vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
		pInfo.vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	}
	
	pInfo.inputAssembly.topology = config.topology_;

	pInfo.colorBlendAttachment.srcAlphaBlendFactor = 
		config.useBlending_ ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE;

	pInfo.depthStencil.depthTestEnable = static_cast<VkBool32>(config.depthTest_ ? VK_TRUE : VK_FALSE);
	pInfo.depthStencil.depthWriteEnable = static_cast<VkBool32>(config.depthWrite_ ? VK_TRUE : VK_FALSE);

	pInfo.tessellationState.patchControlPoints = config.PatchControlPointsCount_;

	// Enable MSAA
	if (config.msaaSamples_ != VK_SAMPLE_COUNT_1_BIT)
	{
		pInfo.multisampling.rasterizationSamples = config.msaaSamples_;
		pInfo.multisampling.sampleShadingEnable = VK_TRUE;
		pInfo.multisampling.minSampleShading = 0.25f;
	}

	// Dynamic viewport and scissor are always active
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	if (config.lineWidth_ > 1.0f)
	{
		dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
	}
	pInfo.dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	pInfo.dynamicState.pDynamicStates = dynamicStates.data();

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pInfo.vertexInputInfo,
		.pInputAssemblyState = &pInfo.inputAssembly,
		.pTessellationState = (config.topology_ == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &pInfo.tessellationState : nullptr,
		.pViewportState = &pInfo.viewportState,
		.pRasterizationState = &pInfo.rasterizer,
		.pMultisampleState = &pInfo.multisampling,
		.pDepthStencilState = config.depthTest_ ? &pInfo.depthStencil : nullptr,
		.pColorBlendState = &pInfo.colorBlending,
		.pDynamicState = &pInfo.dynamicState,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK(vkCreateGraphicsPipelines(
		gd->GetDevice(),
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		pipeline));

	for (Shader& s : shaderModules)
	{
		s.Destroy();
	}
}

void PipelineBase::CreateComputePipeline(const std::string& shaderFile)
{
	Shader shader;
	shader.Create(gd->GetDevice(), shaderFile.c_str());

	const VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {  // ShaderStageInfo
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = shader.GetShaderModule(),
			.pName = "main",
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	VK_CHECK(vkCreateComputePipelines(gd->GetDevice(), 0, 1, &computePipelineCreateInfo, nullptr, &pipeline));

	shader.Destroy();
}