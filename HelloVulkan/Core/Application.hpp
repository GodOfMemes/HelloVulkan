#pragma once

#include "Window.hpp"
#include "Input.hpp"

#include "Graphics/GraphicsDevice.hpp"

struct AppConfig
{
	std::string Title = "Hello Vulkan";
	glm::ivec2 Size = { 1280, 720};
	GraphicsSettings GraphicsSetting;
};

class Application : public WindowCallbackInterface
{
public:
	explicit Application(const AppConfig& config = {});
	virtual ~Application();

	void Run();
protected:
	Window* window;
	double deltaTime = 0;

	GraphicsDevice* graphicsDevice = nullptr;

	virtual void OnLoad() {}
	virtual void OnUpdate(double dt) {}
	virtual void OnRender(VkCommandBuffer commandBuffer) {}
	virtual void OnShutdown() {}

	virtual void OnFramebufferResize(const glm::ivec2& size);

    virtual void OnKey(Keys key, InputAction action, int mods);
    virtual void OnMouse(MouseButton button, InputAction action, int mods);
    virtual void OnMousePos(const glm::vec2& pos);
    virtual void OnMouseScroll(float scroll);
private:
};
