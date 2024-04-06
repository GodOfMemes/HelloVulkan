#include "AABBRenderPipeline.hpp"

AABBRenderPipeline::AABBRenderPipeline(
	GraphicsDevice* ctx,
	SharedResource* resShared,
	MeshScene* scene,
	uint8_t renderBit) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::GraphicsOffScreen,
		.msaaSamples_ = resShared->multiSampledColorImage_.GetSampleCount(),
		.depthTest_ = true,
		.depthWrite_ = false
	}),
	scene_(scene),
	shouldRender_(false)
{
	Buffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
	renderPass.CreateOffScreenRenderPass(renderBit, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(),
		{
			&(resShared->multiSampledColorImage_),
			&(resShared->depthImage_)
		},
		IsOffscreen()
	);
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "AABB/AABBRender.vert",
			SHADER_DIR + "AABB/AABBRender.frag",
		},
		&pipeline
	);
}

AABBRenderPipeline::~AABBRenderPipeline()
{
}

void AABBRenderPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}

	TracyVkZoneC(gd->GetCurrentFrame().TracyContext, commandBuffer, "AABB", tracy::Color::Orange4);

	const uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());
	BindPipeline(commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&descriptorSets_[frameIndex],
		0,
		nullptr);
	const uint32_t boxCount = scene_->GetInstanceCount();
	gd->InsertDebugLabel(commandBuffer, "AABBRenderPipeline", 0xff99ff99);
	vkCmdDraw(commandBuffer, 36, boxCount, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void AABBRenderPipeline::CreateDescriptor()
{
	constexpr uint32_t frameCount = MAX_FRAMES_IN_FLIGHT;

	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 0
	dsInfo.AddBuffer(&(scene_->transformedBoundingBoxBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 1

	// Create pool and layout
	descriptor.CreatePoolAndLayout( dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers[i]), 0);
		descriptor.CreateSet( dsInfo, &(descriptorSets_[i]));
	}
}