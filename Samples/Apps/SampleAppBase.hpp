#pragma once

//#include "Resources/IBLResource.hpp"
#include "Resources/SharedResource.hpp"
#include "UIData.hpp"
#include "Utility/FrameCounter.hpp"
#include <Core/Application.hpp>
#include <Core/CameraController.hpp>

class SampleBase : public Application
{
public:
    explicit SampleBase(const AppConfig& config = {});
protected:
    std::shared_ptr<Camera> camera;
    CameraController controller;
    FrameCounter frameCounter;
    SharedResource* sharedResource;
    UIData uiData;

    void InitSharedResource();
    void OnUpdate(double dt) override;
    void OnRender(VkCommandBuffer commandBuffer) override;
    void OnFramebufferResize(const glm::ivec2 &size) override;
    void OnKey(Keys key, InputAction action, int mods) override;
    void OnMouse(MouseButton button, InputAction action, int mods) override;
    void OnMousePos(const glm::vec2 &pos) override;
};