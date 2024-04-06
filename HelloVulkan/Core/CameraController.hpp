#pragma once

#include "Camera.hpp"
#include "Input.hpp"
#include "imgui.h"

struct CameraController
{
public:
    CameraController(Camera* camera, float cameraSpeed = 2.5, float sensitivity = 0.1f)
        : _camera(camera), 
        _cameraSpeed(cameraSpeed), 
        _sensitivity(sensitivity) {}

    void Update(double dt)
    {
        if(!_camera || ImGui::GetIO().WantCaptureMouse) return;
        float speed = _cameraSpeed * (float)dt;

        glm::vec3 cameraPosition = _camera->GetPosition();

        if(Input::IsKeyDown(Keys::W))
        {
            cameraPosition += speed * _camera->GetFront();
        }
        else if(Input::IsKeyDown(Keys::S))
        {
            cameraPosition -= speed * _camera->GetFront();
        }

        if(Input::IsKeyDown(Keys::A))
        {
            cameraPosition -= glm::normalize(glm::cross(_camera->GetFront(), _camera->GetUp())) * speed;
        }
        else if(Input::IsKeyDown(Keys::D))
        {
            cameraPosition += glm::normalize(glm::cross(_camera->GetFront(), _camera->GetUp())) * speed;
        }

        if(Input::IsKeyDown(Keys::Space))
        {
            cameraPosition += speed * _camera->GetUp();
        }
        else if(Input::IsKeyDown(Keys::C))
        {
            cameraPosition -= speed * _camera->GetUp();
        }

        _camera->SetPosition(cameraPosition);
        return;

        if (_mouseFirstUse)
        {
            _lastMousePos = Input::GetMousePosition();
            _mouseFirstUse = false;
            return;
        }

        if(Input::IsMouseButtonDown(MouseButton::LeftButton))
        {
            glm::vec2 delta = { Input::GetMousePosition().x - _lastMousePos.x, _lastMousePos.y - Input::GetMousePosition().y};
            delta *= _sensitivity;
            _lastMousePos = Input::GetMousePosition();

            float yaw = _camera->GetYaw() + delta.x;
            float pitch = _camera->GetPitch() + delta.y;

            _camera->SetYaw(yaw);
            _camera->SetPitch(std::clamp(pitch,-90.0f,90.0f));
        }

        _camera->SetFOV(std::clamp(_camera->GetFOV() - Input::GetMouseScroll(), 1.f, 45.f));
    }
private:
    Camera* _camera;
    float _cameraSpeed;
    float _sensitivity;
    glm::vec2 _lastMousePos;
    bool _mouseFirstUse = true;
};