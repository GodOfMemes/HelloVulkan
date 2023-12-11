#include "AppTest.h"
#include "AppSettings.h"
#include "VulkanImage.h"
#include "VulkanUtility.h"

struct UBO
{
	glm::mat4 mvp, mv, m;
	glm::vec4 cameraPos;
} ubo;

AppTest::AppTest()
{
}

int AppTest::MainLoop()
{
	std::string cubemapTextureFile = AppSettings::TextureFolder + "piazza_bologni_1k.hdr";
	std::string cubemapIrradianceFile = AppSettings::TextureFolder + "piazza_bologni_1k_irradiance.hdr";

	std::string gltfFile = AppSettings::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf";
	std::string aoFile = AppSettings::ModelFolder + "DamagedHelmet//Default_AO.jpg";
	std::string emissiveFile = AppSettings::ModelFolder + "DamagedHelmet//Default_emissive.jpg";
	std::string albedoFile = AppSettings::ModelFolder + "DamagedHelmet//Default_albedo.jpg";
	std::string roughnessFile = AppSettings::ModelFolder + "DamagedHelmet//Default_metalRoughness.jpg";
	std::string normalFile = AppSettings::ModelFolder + "DamagedHelmet//Default_normal.jpg";

	VulkanImage depthTexture;
	depthTexture.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));
	
	cubePtr = std::make_unique<RendererCube>(vulkanDevice, depthTexture, cubemapTextureFile.c_str());
	clearPtr = std::make_unique<RendererClear>(vulkanDevice, depthTexture);
	finishPtr = std::make_unique<RendererFinish>(vulkanDevice, depthTexture);

	pbrPtr = std::make_unique<RendererPBR>(vulkanDevice,
		(uint32_t)sizeof(UBO),
		gltfFile.c_str(),
		aoFile.c_str(),
		emissiveFile.c_str(),
		albedoFile.c_str(),
		roughnessFile.c_str(),
		normalFile.c_str(),
		cubemapTextureFile.c_str(),
		cubemapIrradianceFile.c_str(),
		depthTexture);

	const std::vector<RendererBase*> renderers = 
	{ 
		clearPtr.get(),
		cubePtr.get(),
		finishPtr.get()
	};

	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();

		const bool frameRendered = DrawFrame(renderers);
	}

	depthTexture.Destroy(vulkanDevice.GetDevice());

	clearPtr = nullptr;
	finishPtr = nullptr;
	cubePtr = nullptr;
	pbrPtr = nullptr;
	Terminate();

	return 0;
}

void AppTest::ComposeFrame(uint32_t imageIndex, const std::vector<RendererBase*>& renderers)
{
	// Renderer
	glm::mat4 model(1.f);
	glm::mat4 projection = camera->GetProjectionMatrix();
	glm::mat4 view = camera->GetViewMatrix();
	cubePtr->UpdateUniformBuffer(vulkanDevice, imageIndex, projection * view * model);

	ubo = UBO
	{
		.mvp = projection * view * model,
		.mv = view * view,
		.m = model,
		.cameraPos = glm::vec4(camera->Position, 1.f) };
	pbrPtr->UpdateUniformBuffer(vulkanDevice, imageIndex, &ubo, sizeof(ubo));

	VkCommandBuffer commandBuffer = vulkanDevice.commandBuffers[imageIndex];

	const VkCommandBufferBeginInfo bi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

	for (auto& r : renderers)
		r->FillCommandBuffer(commandBuffer, imageIndex);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}