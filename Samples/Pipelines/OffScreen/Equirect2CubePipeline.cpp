#include "Equirect2CubePipeline.hpp"
#include "Defines.hpp"
#include "Graphics/Shader.hpp"
#include "Utility/Utility.hpp"

Equirect2CubePipeline::Equirect2CubePipeline(
	GraphicsDevice* gd, 
	const std::string& hdrFile) :
	PipelineBase(
		gd, 
		{
			.type_ = PipelineType::GraphicsOffScreen
		}
	)
{
	InitializeHDRImage(hdrFile);
	renderPass.CreateOffScreenCubemapRenderPass(IBLConfig::CubeFormat);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);
	CreateOffscreenGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "FullscreenTriangle.vert",
			SHADER_DIR + "IBL/Equirect2Cube.frag"
		},
		&pipeline
	);
}

Equirect2CubePipeline::~Equirect2CubePipeline()
{
	inputHDRImage_.Destroy();
	vkDestroyFramebuffer(gd->GetDevice(), cubeFramebuffer_, nullptr);
}

void Equirect2CubePipeline::InitializeCubemap( Texture2D* cubemap)
{
	const uint32_t mipmapCount = Utility::MipMapCount(IBLConfig::InputCubeSideLength);

	cubemap->CreateImage(
		
		IBLConfig::InputCubeSideLength,
		IBLConfig::InputCubeSideLength,
		mipmapCount,
		IBLConfig::LayerCount,
		IBLConfig::CubeFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	cubemap->CreateImageView(
		
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		0u,
		mipmapCount,
		0u,
		IBLConfig::LayerCount);
}

void Equirect2CubePipeline::InitializeHDRImage( const std::string& hdrFile)
{
	inputHDRImage_.CreateFromHDR(hdrFile.c_str());
	inputHDRImage_.CreateImageView(
		
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	inputHDRImage_.CreateDefaultSampler(
		
		0.f,
		1.f);
}

void Equirect2CubePipeline::CreateDescriptor()
{
	DescriptorInfo dsInfo;
	dsInfo.AddImage(&inputHDRImage_);
	descriptor.CreatePoolAndLayout(dsInfo, 1u, 1u);
	descriptor.CreateSet(dsInfo, &descriptorSet_);
}

void Equirect2CubePipeline::CreateOffscreenGraphicsPipeline(
	
	VkRenderPass renderPass,
	VkPipelineLayout pipelineLayout,
	const std::vector<std::string>& shaderFiles,
	VkPipeline* pipeline)
{
	std::vector<Shader> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(shaderFiles.size());
	shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(gd->GetDevice(), file));
		VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// Pipeline create info
	PipelineCreateInfo pInfo{};

	pInfo.viewport.width = IBLConfig::InputCubeSideLength;
	pInfo.viewport.height = IBLConfig::InputCubeSideLength;

	pInfo.scissor.extent = { IBLConfig::InputCubeSideLength, IBLConfig::InputCubeSideLength };

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(IBLConfig::LayerCount, colorBlendAttachment);

	pInfo.colorBlending.attachmentCount = IBLConfig::LayerCount;
	pInfo.colorBlending.pAttachments = colorBlendAttachments.data();

	// No depth test
	pInfo.depthStencil.depthTestEnable = VK_FALSE;
	pInfo.depthStencil.depthWriteEnable = VK_FALSE;
	
	pInfo.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	const VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pInfo.vertexInputInfo,
		.pInputAssemblyState = &pInfo.inputAssembly,
		.pTessellationState = &pInfo.tessellationState,
		.pViewportState = &pInfo.viewportState,
		.pRasterizationState = &pInfo.rasterizer,
		.pMultisampleState = &pInfo.multisampling,
		.pDepthStencilState = &pInfo.depthStencil,
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

	for (auto s : shaderModules)
	{
		s.Destroy();
	}
}

// TODO Can be moved to generic function in PipelineBase
void Equirect2CubePipeline::CreateFramebuffer(
	 
	std::vector<VkImageView>& outputViews)
{
	const VkFramebufferCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = renderPass.GetHandle(),
		.attachmentCount = static_cast<uint32_t>(outputViews.size()),
		.pAttachments = outputViews.data(),
		.width = IBLConfig::InputCubeSideLength,
		.height = IBLConfig::InputCubeSideLength,
		.layers = 1u,
	};

	VK_CHECK(vkCreateFramebuffer(gd->GetDevice(), &info, nullptr, &cubeFramebuffer_));
}

// TODO Move this to Texture2D
void Equirect2CubePipeline::CreateCubemapViews(
	 
	Texture2D* cubemap,
	std::vector<VkImageView>& cubemapViews)
{
	cubemapViews = std::vector<VkImageView>(IBLConfig::LayerCount, VK_NULL_HANDLE);
	for (size_t i = 0; i < IBLConfig::LayerCount; i++)
	{
		const VkImageViewCreateInfo viewInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = cubemap->GetHandle(),
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = cubemap->GetFormat(),
			.components =
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange =
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				1u,
				static_cast<uint32_t>(i),
				1u
			}
		};

		VK_CHECK(vkCreateImageView(gd->GetDevice(), &viewInfo, nullptr, &cubemapViews[i]));
	}
}

void Equirect2CubePipeline::OffscreenRender( Texture2D* outputEnvMap)
{
	// Initialize output cubemap
	InitializeCubemap(outputEnvMap);

	// Create views from the output cubemap
	std::vector<VkImageView> outputViews;
	CreateCubemapViews(outputEnvMap, outputViews);

	CreateFramebuffer(outputViews);

	gd->ImmediateSubmit([&](VkCommandBuffer commandBuffer)
	{
		// Transition
		outputEnvMap->TransitionLayout(
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&descriptorSet_,
			0,
			nullptr);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		renderPass.BeginCubemapRenderPass(commandBuffer, cubeFramebuffer_, IBLConfig::InputCubeSideLength);

		gd->InsertDebugLabel(commandBuffer, "Equirect2CubePipeline", 0xffff99ff);

		vkCmdDraw(commandBuffer, 3, 1u, 0, 0);

		vkCmdEndRenderPass(commandBuffer);
	});

	// Transition
	outputEnvMap->TransitionLayout(
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create a sampler for the output cubemap
	outputEnvMap->CreateDefaultSampler();

	// Destroy image views
	for (size_t i = 0; i < IBLConfig::LayerCount; i++)
	{
		vkDestroyImageView(gd->GetDevice(), outputViews[i], nullptr);
	}
}
