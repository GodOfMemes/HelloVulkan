#include "PBRShadowPipeline.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/GraphicsStructs.hpp"

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 5;
constexpr uint32_t ENV_TEXTURE_COUNT = 4; // Specular, diffuse, BRDF LUT, and shadow map

PBRShadowPipeline::PBRShadowPipeline(GraphicsDevice* gd,
        MeshScene* scene, 
        LightResource* lightRes, 
        IBLResource* iblRes, 
        ShadowResource* shadowRes, 
        SharedResource* sharedRes, 
        MaterialType materialType, 
        uint8_t renderType) :
        PipelineBase(gd, 
            PipelineConfig{
                .type_ = PipelineType::GraphicsOffScreen,
                .msaaSamples_ = sharedRes->multiSampledColorImage_.GetSampleCount(),
                .vertexBufferBind_ = false, // If you use bindless texture, make sure this is false
            }
        ),
        scene(scene),
        bdaBuffer(gd),
        lightResource(lightRes),
        iblResource(iblRes),
        shadowResource(shadowRes),
        materialType(materialType),
        materialOffset(0),
        materialDrawCount(0)
{
    Buffer::CreateMultipleUniformBuffers(gd,cameraUBOBuffers, sizeof(CameraUBO), MAX_FRAMES_IN_FLIGHT);
	Buffer::CreateMultipleUniformBuffers(gd,shadowMapConfigUBOBuffers, sizeof(ShadowMapUBO), MAX_FRAMES_IN_FLIGHT);

	scene->GetOffsetAndDrawCount(materialType, materialOffset, materialDrawCount);

	renderPass.CreateOffScreenRenderPass( renderType, config.msaaSamples_);
	framebuffer.CreateResizeable(
		renderPass.GetHandle(), 
		{
			&sharedRes->multiSampledColorImage_,
			&sharedRes->depthImage_
		}, 
		IsOffscreen());
	PrepareBDA();
	CreateDescriptor();
	CreatePipelineLayout(descriptor.GetLayout(), &pipelineLayout, sizeof(PBRPushConst), VK_SHADER_STAGE_FRAGMENT_BIT);
	CreateSpecializationConstants();
	CreateGraphicsPipeline(
		renderPass.GetHandle(),
		pipelineLayout,
		{
			SHADER_DIR + "ShadowMapping/Scene.vert",
			SHADER_DIR + "ShadowMapping/Scene.frag"
		},
		&pipeline
	);
}

PBRShadowPipeline::~PBRShadowPipeline()
{
    for(auto& buff : shadowMapConfigUBOBuffers)
    {
        buff.Destroy();
    }
    bdaBuffer.Destroy();
}

void PBRShadowPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    if (materialDrawCount == 0)
	{
		return;
	}

    auto frame = gd->GetCurrentFrame();

	TracyVkZoneC(frame.TracyContext, commandBuffer, "PBR_Shadow", tracy::Color::Aqua);
	
	uint32_t frameIndex = gd->GetCurrentFrameIdx();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer());

	BindPipeline(commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PBRPushConst), &pc);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0u,
		1u,
		&descriptorSets[frameIndex],
		0u,
		nullptr);

	gd->InsertDebugLabel(commandBuffer, "PipelinePBRShadow", 0xff9999ff);

	vkCmdDrawIndirect(
		commandBuffer,
		scene->indirectBuffer_.GetHandle(),
		materialOffset,
		materialDrawCount,
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

void PBRShadowPipeline::SetShadowMapConfigUBO(ShadowMapUBO& ubo)
{
    shadowMapConfigUBOBuffers[gd->GetCurrentFrameIdx()].UploadBufferData(&ubo, sizeof(ShadowMapUBO));
}

void PBRShadowPipeline::PrepareBDA()
{
    BDA bda = scene->GetBDA();
	VkDeviceSize bdaSize = sizeof(BDA);
	bdaBuffer.CreateBuffer(
		bdaSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	bdaBuffer.UploadBufferData( &bda, bdaSize);
}

void PBRShadowPipeline::CreateDescriptor()
{
	DescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 1
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 2
	dsInfo.AddBuffer(&bdaBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 3
	dsInfo.AddBuffer(lightResource->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 4
	dsInfo.AddImage(&iblResource->specularCubemap_); // 5
	dsInfo.AddImage(&iblResource->diffuseCubemap_); // 6
	dsInfo.AddImage(&iblResource->brdfLut_); // 7
	dsInfo.AddImage(&shadowResource->shadowMap_); // 8
	dsInfo.AddImageArray(scene->GetImageInfos()); // 9

	// Pool and layout
	descriptor.CreatePoolAndLayout( dsInfo, MAX_FRAMES_IN_FLIGHT, 1u);

	// Sets
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		dsInfo.UpdateBuffer(&cameraUBOBuffers[i], 0);
		dsInfo.UpdateBuffer(&shadowMapConfigUBOBuffers[i], 1);
		dsInfo.UpdateBuffer(&scene->modelSSBOBuffers_[i], 2);
		descriptor.CreateSet(dsInfo, &descriptorSets[i]);
	}
}

void PBRShadowPipeline::CreateSpecializationConstants()
{
    alphaDiscard = materialType == MaterialType::Transparent ? 1u : 0u;

	std::vector<VkSpecializationMapEntry> specializationEntries = {{
		.constantID = 0,
		.offset = 0,
		.size = sizeof(uint32_t)
	}};

	specializationConstants.ConsumeEntries(
		std::move(specializationEntries),
		&alphaDiscard,
		sizeof(uint32_t),
		VK_SHADER_STAGE_FRAGMENT_BIT);
}
