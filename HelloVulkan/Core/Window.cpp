#include "Window.hpp"

#include <GLFW/glfw3.h>

Window::Window(const std::string& title, const glm::ivec2& windowSize)
    : _title(title), _size(windowSize)
{
    _window = glfwCreateWindow(windowSize.x, windowSize.y, title.c_str(), nullptr, nullptr);
    if(!_window) throw std::runtime_error("Failed to create GLFW window");
    glfwSetWindowUserPointer(_window, this);
    SetupCallback();
}

Window::~Window()
{
    glfwDestroyWindow(_window);
}

void Window::SetUserPointer(WindowCallbackInterface* callbackInterface)
{
    _callbackInterface = callbackInterface;
}

void Window::Close()
{
    glfwSetWindowShouldClose(_window, true);
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(_window);
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance)
{
    VkSurfaceKHR surface;
    if(glfwCreateWindowSurface(instance, _window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}
    return surface;
}

void Window::ShowWindow()
{
    glfwShowWindow(_window);
}

void Window::HideWindow()
{
    glfwHideWindow(_window);
}

void Window::SetupCallback()
{
    glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* win, int width, int height)
    {
        auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
        if(!window) return;
        window->_size = {width, height};
        if(window->_callbackInterface)
        {
            window->_callbackInterface->OnFramebufferResize({width,height});
        }
    });

    glfwSetKeyCallback(_window, [](GLFWwindow* win, int key, int scancode, int action, int mods)
	{
		auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
        if(!window) return;
        if(window->_callbackInterface)
        {
            window->_callbackInterface->OnKey((Keys)key,(InputAction)action,mods);
        }
	});

	glfwSetMouseButtonCallback(_window, [](GLFWwindow* win, int button, int action, int mods)
    {
        auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
        if(!window) return;
        if(window->_callbackInterface)
        {
            window->_callbackInterface->OnMouse((MouseButton)button,(InputAction)action,mods);
        }
    });

	glfwSetCursorPosCallback(_window, [](GLFWwindow* win, double xpos, double ypos)
    {
        auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
        if(!window) return;
        if(window->_callbackInterface)
        {
            window->_callbackInterface->OnMousePos({xpos,ypos});
        }
    });

	glfwSetScrollCallback(_window, [](GLFWwindow* win, double xoffset, double yoffset)
    {
        auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
        if(!window) return;
        if(window->_callbackInterface)
        {
            window->_callbackInterface->OnMouseScroll(yoffset);
        }
    });
}
