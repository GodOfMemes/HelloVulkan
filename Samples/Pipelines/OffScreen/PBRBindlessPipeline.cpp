#include "PBRBindlessPipeline.hpp"

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 2;
constexpr uint32_t ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PBRBindlessPipeline::PBRBindlessPipeline(
	GraphicsDevice* ctx,
	MeshScene* scene,
	LightResource* resourcesLight,
	IBLResource* resourcesIBL,
	SharedResource* resourcesShared,
	bool useSkinning,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.GetSampleCount(),
			.vertexBufferBind_ = false,
		}
	),
	scene_(scene),
	resourcesLight_(resourcesLight),
	resourcesIBL_(resourcesIBL),
	useSkinning_(useSkinning),
    bdaBuffer_(gd)
{
	Buffer::CreateMultipleUniformBuffers(gd,cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
	PrepareBDA(); // Buffer device address
	CreateDescriptor();
	renderPass.CreateOffScreenRenderPass(renderBit, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(), 
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		}, 
		IsOffscreen());
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout, sizeof(PBRPushConst), VK_SHADER_STAGE_FRAGMENT_BIT);
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "Bindless/Scene.vert",
			SHADER_DIR + "Bindless/Scene.frag"
		},
		&pipeline
	);
}

PBRBindlessPipeline::~PBRBindlessPipeline()
{
	bdaBuffer_.Destroy();
}

void PBRBindlessPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();
	TracyVkZoneC(frame.TracyContext, commandBuffer, "PBR_Bindless", tracy::Color::LimeGreen);

	const uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());

	BindPipeline(commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PBRPushConst), &pc_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0u, // firstSet
		1u, // descriptorSetCount
		&(descriptorSets_[frameIndex]),
		0u, // dynamicOffsetCount
		nullptr); // pDynamicOffsets

	gd->InsertDebugLabel(commandBuffer, "PBRBindlessPipeline", 0xff9999ff);

	vkCmdDrawIndirect(
		commandBuffer, 
		scene_->indirectBuffer_.GetHandle(), 
		0, // offset
		scene_->GetInstanceCount(),
		sizeof(VkDrawIndirectCommand));
	
	vkCmdEndRenderPass(commandBuffer);
}

void PBRBindlessPipeline::PrepareBDA()
{
	BDA bda = scene_->GetBDA();
	VkDeviceSize bdaSize = sizeof(BDA);
	bdaBuffer_.CreateBuffer(
		bdaSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	bdaBuffer_.UploadBufferData( &bda, bdaSize);
}

void PBRBindlessPipeline::CreateDescriptor()
{
	constexpr uint32_t frameCount = MAX_FRAMES_IN_FLIGHT;

	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	dsInfo.AddBuffer(&bdaBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 2
	dsInfo.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // 3
	dsInfo.AddImage(&(resourcesIBL_->specularCubemap_)); // 4
	dsInfo.AddImage(&(resourcesIBL_->diffuseCubemap_)); // 5
	dsInfo.AddImage(&(resourcesIBL_->brdfLut_)); // 6
	dsInfo.AddImageArray(scene_->GetImageInfos()); // 7

	// Pool and layout
	descriptor.CreatePoolAndLayout( dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		descriptor.CreateSet( dsInfo, &(descriptorSets_[i]));
	}
}