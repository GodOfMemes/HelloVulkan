#include "SampleAppBase.hpp"
#include <imgui.h>

SampleBase::SampleBase(const AppConfig& config)
    : Application(config),
    camera(std::make_shared<Camera>(glm::vec3(0,0,3))),
    controller(camera.get())
{
    //InitSharedResource();
}

void SampleBase::InitSharedResource()
{
    if(!sharedResource)
    {
        sharedResource = graphicsDevice->AddResources<SharedResource>(graphicsDevice);
    }
    sharedResource->Create();
}

void SampleBase::OnUpdate(double dt)
{
    frameCounter.Update(dt);
    //controller.Update(dt);

    if (uiData.mouseLeftPressed_)
	{
		uiData.mouseLeftPressed_ = false;
		uiData.mouseLeftHold_ = true;
	}
}

void SampleBase::OnRender(VkCommandBuffer commandBuffer)
{
    
}

void SampleBase::OnFramebufferResize(const glm::ivec2 &size)
{
    Application::OnFramebufferResize(size);

    camera->SetAspectRatio((float)size.x / size.y);
    InitSharedResource();
}

void SampleBase::OnKey(Keys key, InputAction action, int mods)
{
    Application::OnKey(key,action,mods);

    if(key == Keys::I && action == InputAction::Press)
    {
        uiData.imguiShow_ = !uiData.imguiShow_;
    }
    else if(key == Keys::Escape && action == InputAction::Press)
    {
        window->Close();
    }
}

void SampleBase::OnMouse(MouseButton button, InputAction action, int mods)
{
    Application::OnMouse(button, action, mods);

    if (!ImGui::GetIO().WantCaptureMouse && button == MouseButton::LeftButton && action == InputAction::Press)
	{
		uiData.mouseLeftPressed_ = true;
	}
    else if (button == MouseButton::LeftButton && action == InputAction::Release)
	{
		uiData.mouseLeftPressed_ = false;
		uiData.mouseLeftHold_ = false;
	}
}

void SampleBase::OnMousePos(const glm::vec2 &pos)
{
    Application::OnMousePos(pos);
}
