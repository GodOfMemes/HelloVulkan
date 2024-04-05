#pragma once

#include "Input.hpp"
#include "Minimal.hpp"
#include <volk.h> //<vulkan/vulkan.h> 

struct GLFWwindow;

class WindowCallbackInterface
{
    friend class Window;
protected:
    virtual void OnFramebufferResize(const glm::ivec2& size) = 0;

    virtual void OnKey(Keys key, InputAction action, int mods) = 0;
    virtual void OnMouse(MouseButton button, InputAction action, int mods) = 0;
    virtual void OnMousePos(const glm::vec2& pos) = 0;
    virtual void OnMouseScroll(float scroll) = 0;
};

class Window final
{
public:
    explicit Window(const std::string& title, const glm::ivec2& windowSize);
    ~Window();

    void SetUserPointer(WindowCallbackInterface* callbackInterface);

    void Close();
    [[nodiscard]] bool ShouldClose();

    [[nodiscard]] std::string GetTitle() const { return _title; }
    [[nodiscard]] glm::ivec2 GetSize() const { return _size; }
    [[nodiscard]] GLFWwindow* GetHandle() const { return _window; }

    [[nodiscard]] VkSurfaceKHR CreateSurface(VkInstance instance);

    void ShowWindow();
    void HideWindow();
private:
    GLFWwindow* _window;
    std::string _title;
    glm::ivec2 _size;
    WindowCallbackInterface* _callbackInterface;

    void SetupCallback();
};