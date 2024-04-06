#include "ShadowPipeline.hpp"
#include <Graphics/Mesh/MeshScene.hpp>
#include <vulkan/vulkan_core.h>
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/GraphicsStructs.hpp"
#include "Graphics/RenderPass.hpp"
#include "Resources/ShadowResource.hpp"

ShadowPipeline::ShadowPipeline(GraphicsDevice* gd, MeshScene* scene, ShadowResource* shadResource)
    : PipelineBase(gd,
    PipelineConfig
    {
        .type_ = PipelineType::GraphicsOffScreen,
        .vertexBufferBind_ = false,
        .customViewportSize_ = true,
        .viewportSize = shadResource->shadowMap_.GetSize(),
    }),
    bda(scene->GetBDA()),
    scene(scene),
    shadowResource(shadResource),
    indirectBuffer(gd)
{
    Buffer::CreateMultipleUniformBuffers(gd, shadowMapUBOBuffers, sizeof(ShadowMapUBO), MAX_FRAMES_IN_FLIGHT);
    renderPass.CreateDepthOnlyRenderPass(RenderPassType::DepthClear | RenderPassType::DepthShaderReadOnly);
    framebuffer.CreateUnresizeable(
        renderPass.GetHandle(), 
        {shadowResource->shadowMap_.GetView()},
        shadowResource->shadowMap_.GetSize().width,
        shadowResource->shadowMap_.GetSize().height);
    scene->CreateIndirectBuffer(indirectBuffer);
    CreateDescriptor();
    CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout, sizeof(BDA), VK_SHADER_STAGE_VERTEX_BIT);
    CreateGraphicsPipeline(renderPass.GetHandle(), pipelineLayout,
        {
            SHADER_DIR + "ShadowMapping/Depth.vert",
            SHADER_DIR + "ShadowMapping/Depth.frag"
        }, &pipeline);
}

ShadowPipeline::~ShadowPipeline()
{
    for(auto& buffer : shadowMapUBOBuffers)
    {
        buffer.Destroy();
    }
    indirectBuffer.Destroy();
}

void ShadowPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();
    TracyVkZoneC(frame.TracyContext, commandBuffer, "Render_Shadow_Map", tracy::Color::OrangeRed);

	const uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(
		commandBuffer, 
		framebuffer.GetFramebuffer(), 
		shadowResource->shadowMap_.GetSize());
	BindPipeline( commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(BDA), &bda);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0u,
		1u,
		&descriptorSets[frameIndex],
		0u,
		nullptr);

	gd->InsertDebugLabel(commandBuffer, "PipelineShadow", 0xff99ff99);

	vkCmdDrawIndirect(
		commandBuffer,
		indirectBuffer.GetHandle(),
		0, // offset
		scene->GetInstanceCount(),
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

void ShadowPipeline::UpdateShadow(ShadowResource* shadowResource, glm::vec4 lightPosition)
{
    glm::mat4 lightProjection = glm::ortho(
		-shadowResource->orthoSize_,
		shadowResource->orthoSize_,
		shadowResource->orthoSize_,
		-shadowResource->orthoSize_,
		shadowResource->shadowNearPlane_,
		shadowResource->shadowFarPlane_);
	glm::mat4 lightView = glm::lookAt(glm::vec3(lightPosition), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	shadowResource->shadowUBO_.lightSpaceMatrix = lightSpaceMatrix;
	shadowResource->shadowUBO_.lightPosition = lightPosition;

	shadowMapUBOBuffers[gd->GetCurrentFrameIdx()].UploadBufferData(&shadowResource->shadowUBO_, sizeof(ShadowMapUBO));
}

void ShadowPipeline::CreateDescriptor()
{
	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1

	// Pool and layout
	descriptor.CreatePoolAndLayout(dsInfo, MAX_FRAMES_IN_FLIGHT, 1u);

	// Sets
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dsInfo.UpdateBuffer(&shadowMapUBOBuffers[i], 0);
		dsInfo.UpdateBuffer(&scene->modelSSBOBuffers_[i], 1);
		descriptor.CreateSet(dsInfo, &descriptorSets[i]);
	}
}
