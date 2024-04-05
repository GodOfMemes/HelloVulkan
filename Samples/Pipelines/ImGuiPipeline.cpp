#include "ImGuiPipeline.hpp"
#include "Defines.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "UIData.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

ImGuiPipeline::ImGuiPipeline(GraphicsDevice* gd, Window* window, MeshScene* scene, Camera* camera)
    : PipelineBase(gd, { .type_ = PipelineType::GraphicsOnScreen }),
        scene(scene),
        window(window),
        camera(camera)
{
    // Create render pass
	renderPass.CreateOnScreenColorOnlyRenderPass();

	// Create framebuffer
	framebuffer.CreateResizeable(renderPass.GetHandle(), {}, IsOffscreen());

	// Create a decsiptor pool
	DescriptorInfo dsInfo;
	dsInfo.AddImage(nullptr); // NOTE According to Sascha Willems, we only need one image, if error, then we need to use vkguide.dev code
	descriptor.CreatePoolAndLayout(dsInfo, MAX_FRAMES_IN_FLIGHT, 1u, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.7f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.21f, 0.61f, 0.61f, 1.00f);

	ImGuiIO& io = ImGui::GetIO();
	const std::string filename = FONT_DIR + "Roboto-Medium.ttf";
	io.Fonts->AddFontFromFileTTF(filename.c_str(), 18.0f);

	// Known issue when using both ImGui and volk
	// github.com/ocornut/imgui/issues/4854

	ImGui_ImplVulkan_LoadFunctions(
        [](const char* functionName, void* userData)
		{
			GraphicsDevice* graphicsDevice = *reinterpret_cast<GraphicsDevice**>(userData);
			return vkGetInstanceProcAddr(graphicsDevice->GetInstance(), functionName);
		}, &gd);

	ImGui_ImplGlfw_InitForVulkan(window->GetHandle(), true);

	ImGui_ImplVulkan_InitInfo init_info = {
		.Instance = gd->GetInstance(),
		.PhysicalDevice = gd->GetPhysicalDevice(),
		.Device = gd->GetDevice(),
		.QueueFamily = gd->GetGraphicsFamily(),
		.Queue = gd->GetGraphicsQueue(),
		.PipelineCache = nullptr,
		.DescriptorPool = descriptor.GetPool(),
		.Subpass = 0,
		.MinImageCount = MAX_FRAMES_IN_FLIGHT,
		.ImageCount = MAX_FRAMES_IN_FLIGHT,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.ColorAttachmentFormat = gd->GetImageFormat(),
	};

	ImGui_ImplVulkan_Init(&init_info, renderPass.GetHandle());
}

ImGuiPipeline::~ImGuiPipeline()
{
    ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiPipeline::ImGuiStart()
{
    ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiPipeline::ImGuiSetWindow(const char* title, int width, int height, float fontSize)
{
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
	ImGui::Begin(title);
	ImGui::SetWindowFontScale(fontSize);
}

void ImGuiPipeline::ImGuiShowFrameData(FrameCounter* frameCounter)
{
    ImGui::Text("FPS: %.0f", frameCounter->GetCurrentFPS());
	ImGui::Text("Delta: %.0f ms", frameCounter->GetDelayedDeltaMillisecond());
	ImGui::PlotLines("FPS",
		frameCounter->GetGraph(),
		frameCounter->GetGraphLength(),
		0,
		nullptr,
		FLT_MAX,
		FLT_MAX,
		ImVec2(350, 50));
}

void ImGuiPipeline::ImGuiShowPBRConfig(PBRPushConst* pc, float mipmapCount)
{
    ImGui::SliderFloat("Light Falloff", &pc->lightFalloff, 0.01f, 5.f);
	ImGui::SliderFloat("Light Intensity", &pc->lightIntensity, 0.1f, 20.f);
	ImGui::SliderFloat("Albedo", &pc->albedoMultipler, 0.0f, 1.0f);
	ImGui::SliderFloat("Reflectivity", &pc->baseReflectivity, 0.01f, 1.f);
	ImGui::SliderFloat("Max Lod", &pc->maxReflectionLod, 0.1f, mipmapCount);
}

void ImGuiPipeline::ImGuiEnd()
{
    ImGui::End();
	ImGui::Render();
}

void ImGuiPipeline::ImGuiDrawEmpty()
{
    ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Render();
}

void ImGuiPipeline::ImGuizmoStart()
{
    ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();
	ImGuizmo::SetGizmoSizeClipSpace(0.15f);
}

void ImGuiPipeline::ImGuizmoShow(glm::mat4& modelMatrix, const int editMode)
{
    if (editMode == GizmoMode::None)
	{
		return;
	}

	static ImGuizmo::OPERATION gizmoOperation = ImGuizmo::TRANSLATE;

	gizmoOperation = editMode == GizmoMode::Translate ? ImGuizmo::TRANSLATE :
		(editMode == GizmoMode::Rotate ? ImGuizmo::ROTATE : ImGuizmo::SCALE);

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 projection = camera->GetProjectionMatrix();
	projection[1][1] *= -1;

	// Note that modelMatrix is modified by the function below
	ImGuizmo::Manipulate(
		glm::value_ptr(view),
		glm::value_ptr(projection),
		gizmoOperation,
		ImGuizmo::WORLD,
		glm::value_ptr(modelMatrix));
}

void ImGuiPipeline::ImGuizmoShowOption(int* editMode)
{
    ImGui::Text("Edit Mode ");
	ImGui::RadioButton("None", editMode, 0); ImGui::SameLine();
	ImGui::RadioButton("Translate", editMode, 1); ImGui::SameLine();
	ImGui::RadioButton("Rotate", editMode, 2); ImGui::SameLine();
	ImGui::RadioButton("Scale", editMode, 3);
}

void ImGuiPipeline::ImGuizmoManipulateScene(UIData* uiData)
{
    if (!scene  || !camera)
	{
		return;
	}

	if (uiData->GizmoCanSelect())
	{
		Ray r = camera->GetRayFromScreenToWorld(window->GetSize(),Input::GetMousePosition());
		int i = scene ->GetClickedInstanceIndex(r);
		if (i >= 0)
		{
			InstanceData& iData = scene ->instanceDataArray_[i];
			uiData->gizmoModelIndex = iData.meshData_.modelMatrixIndex_;
			uiData->gizmoInstanceIndex = i;
		}
	}

	if (uiData->GizmoActive())
	{
		ImGuizmoStart();
		ImGuizmoShow(
			scene ->modelSSBOs_[uiData->gizmoModelIndex].model,
			uiData->gizmoMode_);

		// TODO Code smell because UI directly manipulates the scene
		const InstanceData& iData = scene ->instanceDataArray_[uiData->gizmoInstanceIndex];
		scene ->UpdateModelMatrixBuffer( iData.modelIndex_, iData.perModelInstanceIndex_);
	}
}

void ImGuiPipeline::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto frame = gd->GetCurrentFrame();

    TracyVkZoneC(frame.TracyContext, commandBuffer, "ImGui", tracy::Color::DarkSeaGreen);
	ImDrawData* draw_data = ImGui::GetDrawData();
	renderPass.BeginRenderPass(commandBuffer, framebuffer.GetFramebuffer(frame.imageIndex));
	gd->InsertDebugLabel(commandBuffer, "PipelineImGui", 0xff9999ff);
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}
