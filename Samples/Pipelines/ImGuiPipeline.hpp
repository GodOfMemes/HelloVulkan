#pragma once

#include "Core/Window.hpp"
#include <Graphics/GraphicsStructs.hpp>
#include "UIData.hpp"
#include "Utility/FrameCounter.hpp"
#include <Core/Camera.hpp>
#include <Graphics/Mesh/MeshScene.hpp>
#include <Graphics/Pipeline.hpp>
#include <Graphics/GraphicsDevice.hpp>

class ImGuiPipeline : public PipelineBase
{
public:
    ImGuiPipeline(GraphicsDevice* gd, Window* window, MeshScene* scene, Camera* camera);
    ~ImGuiPipeline();

    void ImGuiStart();
	void ImGuiSetWindow(const char* title, int width, int height, float fontSize = 1.0f);
	void ImGuiShowFrameData(FrameCounter* frameCounter);
	void ImGuiShowPBRConfig(PBRPushConst* pc, float mipmapCount);
	void ImGuiEnd();
	void ImGuiDrawEmpty();

	void ImGuizmoStart();
	void ImGuizmoShow(glm::mat4& modelMatrix, const int editMode);
	void ImGuizmoShowOption(int* editMode);
    void ImGuizmoManipulateScene(UIData* uiData);

    void FillCommandBuffer(VkCommandBuffer commandBuffer) override;
private:
    MeshScene* scene;
    Window* window;
    Camera* camera;
};