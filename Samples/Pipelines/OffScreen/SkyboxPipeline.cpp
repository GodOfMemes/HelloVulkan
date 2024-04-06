#include "SkyboxPipeline.hpp"
#include "Defines.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/GraphicsStructs.hpp"
#include <vulkan/vulkan_core.h>

SkyBoxPipeline::SkyBoxPipeline(GraphicsDevice* gd,
        Texture2D* envMap,
        SharedResource* sharedResource, 
        uint8_t type)
    :    PipelineBase(gd,
            PipelineConfig
            {
                .type_ = PipelineType::GraphicsOffScreen,
                .msaaSamples_ = sharedResource->multiSampledColorImage_.GetSampleCount(),
                .vertexBufferBind_ = false,
                .depthTest_ = true,
                .depthWrite_ = false
            }),
            envCubeMap(envMap)
{
    Buffer::CreateMultipleUniformBuffers(gd, cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);

    // Note that this pipeline is offscreen rendering
	renderPass.CreateOffScreenRenderPass( type, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(),
		{
			&sharedResource->multiSampledColorImage_,
			&sharedResource->depthImage_
		},
		IsOffscreen()
	);

	CreateDescriptor();
	
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout);

	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "Skybox.vert",
			SHADER_DIR + "Skybox.frag",
		},
		&pipeline
		); 
}

void SkyBoxPipeline::SetCameraUBO(CameraUBO& ubo)
{
    // Remove translation
    CameraUBO skyboxUbo = ubo;
    skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
    PipelineBase::SetCameraUBO(skyboxUbo);
}

void SkyBoxPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();

    TracyVkZoneC(frame.TracyContext, commandBuffer, "Skybox", tracy::Color::BlueViolet);
	uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());
	BindPipeline(commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		pipelineLayout, 
		0, 
		1, 
		&descriptorSets[frameIndex],
		0, 
		nullptr);
	gd->InsertDebugLabel(commandBuffer, "PipelineSkybox", 0xff99ff99);
	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void SkyBoxPipeline::CreateDescriptor()
{
    DescriptorInfo dsInfo;
    dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    dsInfo.AddImage(envCubeMap);

    descriptor.CreatePoolAndLayout(dsInfo, MAX_FRAMES_IN_FLIGHT, 1);
    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        dsInfo.UpdateBuffer(&cameraUBOBuffers[i], 0);
        descriptor.CreateSet(dsInfo, &descriptorSets[i]);
    }
}