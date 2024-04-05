#include "Application.hpp"
#include "GLFW/glfw3.h"

Application::Application(const AppConfig& config)
{
	glfwSetErrorCallback([](int error, const char* description)
	{
		throw std::runtime_error(std::format("GLFW Error({}): {}",error,description));
	});

	if(!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	window = new Window(config.Title,config.Size);
	window->SetUserPointer(this);
	graphicsDevice = new GraphicsDevice(window,config.GraphicsSetting);
}

Application::~Application()
{
	delete graphicsDevice;
	delete window;
    glfwTerminate();
}

void Application::Run()
{
	OnLoad();
	window->ShowWindow();
	double lastTime = glfwGetTime();

	auto renderCallback = std::bind(&Application::OnRender, this, std::placeholders::_1);
	while(!window->ShouldClose())
	{
		double currentTime = glfwGetTime();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		glfwPollEvents();
		OnUpdate(deltaTime);
		graphicsDevice->DrawFrame(renderCallback);
		//glfwPollEvents();
	}
	window->HideWindow();
	graphicsDevice->WaitForIdle();
	OnShutdown();
}

void Application::OnFramebufferResize(const glm::ivec2& size)
{
	graphicsDevice->HandleResize();
}

void Application::OnKey(Keys key, InputAction action, int mods)
{
	Input::m_keys[(int)key] = action;
}

void Application::OnMouse(MouseButton button, InputAction action, int mods)
{
	Input::m_mouseButtons[(int)button] = action;
}

void Application::OnMousePos(const glm::vec2& pos)
{
	Input::m_mousePosition = pos;
}

void Application::OnMouseScroll(float scroll)
{
	Input::m_mouseScroll = scroll;
}