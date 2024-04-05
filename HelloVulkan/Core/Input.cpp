#include "Input.hpp"

InputAction Input::m_keys[static_cast<int>(Keys::COUNT)];
InputAction Input::m_mouseButtons[static_cast<int>(MouseButton::COUNT)];
glm::vec2 Input::m_mousePosition;
float Input::m_mouseScroll;