#include "PBRClusterForwardPipeline.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/GraphicsStructs.hpp"

// Constants
constexpr uint32_t UBO_COUNT = 3;
constexpr uint32_t SSBO_COUNT = 3;
constexpr uint32_t PBR_TEXTURE_COUNT = 6;
constexpr uint32_t ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PBRClusterForwardPipeline::PBRClusterForwardPipeline(
	GraphicsDevice* ctx,
	MeshScene* scene,
	LightResource* lights,
	ClusterForwardResource* resourcesCF,
	IBLResource* resourcesIBL,
	SharedResource* resourcesShared,
	MaterialType materialType,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.GetSampleCount(),
			.vertexBufferBind_ = false,
		}),
	scene_(scene),
	resourcesLight_(lights),
	resourcesCF_(resourcesCF),
	resourcesIBL_(resourcesIBL),
	materialType_(materialType),
	materialOffset_(0),
	materialDrawCount_(0),
	bdaBuffer_(gd)
{
	Buffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
	Buffer::CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), MAX_FRAMES_IN_FLIGHT);

	scene_->GetOffsetAndDrawCount(materialType_, materialOffset_, materialDrawCount_);

	renderPass.CreateOffScreenRenderPass(renderBit, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(),
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		},
		IsOffscreen());
	PrepareBDA(); // Buffer device address
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout, sizeof(PBRPushConst), VK_SHADER_STAGE_FRAGMENT_BIT);
	CreateSpecializationConstants();
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "Bindless/Scene.vert",
			SHADER_DIR + "ClusteredForward/Scene.frag"
		},
		&pipeline
	);
}

PBRClusterForwardPipeline::~PBRClusterForwardPipeline()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
	bdaBuffer_.Destroy();
}

void PBRClusterForwardPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(gd->GetCurrentFrame().TracyContext, commandBuffer, "PBR_Cluster_Forward", tracy::Color::MediumPurple);

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

	gd->InsertDebugLabel(commandBuffer, "PipelinePBRClusteredForward", 0xff9999ff);

	vkCmdDrawIndirect(
		commandBuffer,
		scene_->indirectBuffer_.GetHandle(),
		materialOffset_,
		materialDrawCount_,
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

void PBRClusterForwardPipeline::PrepareBDA()
{
	BDA bda = scene_->GetBDA();
	VkDeviceSize bdaSize = sizeof(BDA);
	bdaBuffer_.CreateBuffer(
		bdaSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	bdaBuffer_.UploadBufferData(&bda, bdaSize);
}

void PBRClusterForwardPipeline::CreateSpecializationConstants()
{
	alphaDiscard_ = materialType_ == MaterialType::Transparent ? 1u : 0u;

	std::vector<VkSpecializationMapEntry> specializationEntries = { {
		.constantID = 0,
		.offset = 0,
		.size = sizeof(uint32_t)
	} };

	specializationConstants.ConsumeEntries(
		std::move(specializationEntries),
		&alphaDiscard_,
		sizeof(uint32_t),
		VK_SHADER_STAGE_FRAGMENT_BIT);
}

void PBRClusterForwardPipeline::CreateDescriptor()
{
	constexpr uint32_t frameCount = MAX_FRAMES_IN_FLIGHT;

	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	dsInfo.AddBuffer(&bdaBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 2
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 3
	dsInfo.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // 4
	dsInfo.AddBuffer(&(resourcesCF_->lightCellsBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 5
	dsInfo.AddBuffer(&(resourcesCF_->lightIndicesBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 6
	dsInfo.AddImage(&(resourcesIBL_->specularCubemap_)); // 7
	dsInfo.AddImage(&(resourcesIBL_->diffuseCubemap_)); // 8
	dsInfo.AddImage(&(resourcesIBL_->brdfLut_)); // 9
	dsInfo.AddImageArray(scene_->GetImageInfos()); // 10

	// Pool and layout
	descriptor.CreatePoolAndLayout( dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		dsInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 3);
		descriptor.CreateSet( dsInfo, &(descriptorSets_[i]));
	}
}