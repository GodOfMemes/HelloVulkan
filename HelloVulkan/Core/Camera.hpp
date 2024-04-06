#pragma once

#include "Math/Ray.hpp"
#include "Minimal.hpp"
#include <glm/matrix.hpp>

class Camera
{
public:
    Camera(const glm::vec3& position = glm::vec3(),
        float aspectRatio = 16.0f / 9.0f,
        const glm::vec3& worldUp = glm::vec3(0, 1, 0));
        
    void SetPosition(const glm::vec3& position)
    {
        this->position = position;
        RecalculateMatrices();
    }

    glm::vec3 GetPosition() const { return position; }

    void SetFront(const glm::vec3& front)
    {
        this->front = glm::normalize(front);
        RecalculateMatrices();
    }

    glm::vec3 GetFront() const { return front; }

    void SetUp(const glm::vec3& up)
    {
        this->up = glm::normalize(up);
        RecalculateMatrices();
    }
    glm::vec3 GetUp() const { return up; }

    glm::mat4 GetViewMatrix() const { return viewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return projectionMatrix; }
    glm::mat4 GetInvProjectionMatrix() const { return invProjectionMatrix; }

    bool IsOrthographics() const { return isOrthographic; }
    void SetOrthographics(bool isOrthographic) 
    { 
        this->isOrthographic = isOrthographic; 
        RecalculateMatrices();
    }

    void SetOrthoSize(const glm::vec2& size)
    {
        orthoWidth = size.x;
        orthoHeight = size.y;
        RecalculateMatrices();
    }

    glm::vec2 GetOrthoSize() const { return glm::vec2(orthoWidth, orthoHeight); }

    void SetAspectRatio(float aspectRatio)
    {
        this->aspectRatio = aspectRatio;
        RecalculateMatrices();
    }

    float GetNearPlane() const { return near; }
    void SetNearPlane(float near) 
    { 
        this->near = near; 
        UpdateCameraVectors();
        RecalculateMatrices();
    }

    float GetFarPlane() const { return far; }
    void SetFarPlane(float far) 
    { 
        this->far = far; 
        UpdateCameraVectors();
        RecalculateMatrices();
    }

    float GetFOV() const { return fov; }
    void SetFOV(float fov) 
    { 
        this->fov = fov; 
        UpdateCameraVectors();
        RecalculateMatrices();
    }

    float GetYaw() const { return yaw; }
    void SetYaw(float yaw) 
    { 
        this->yaw = yaw; 
        UpdateCameraVectors();
        RecalculateMatrices();
    }

    float GetPitch() const { return pitch; }
    void SetPitch(float pitch) 
    { 
        this->pitch = pitch;
        UpdateCameraVectors();
        RecalculateMatrices();
    }

    [[nodiscard]] Ray GetRayFromScreenToWorld(const glm::vec2& pos) const
    {
        return GetRayFromScreenToWorld(GetOrthoSize(),pos);
    }

    [[nodiscard]] Ray GetRayFromScreenToWorld(const glm::vec2& screenSize,const glm::vec2& pos) const
    {
        glm::mat4 invView = glm::inverse(viewMatrix);
        glm::vec4 rayClip = {
            (pos.x / screenSize.x) * 2 - 1, 
            (pos.y / screenSize.y) * 2 - 1, 
            -1, 1};

        glm::vec4 rayView = invProjectionMatrix * rayClip;
        rayView.z = -1;
        rayView.w = 0;

        glm::vec4 rayWorld = glm::normalize(invView * rayView);
        return {position, glm::vec3(rayWorld)};
    }

    void SetPositionAndTarget(glm::vec3 cameraPosition, glm::vec3 cameraTarget)
    {
        position = cameraPosition;

        const glm::vec3 direction = glm::normalize(cameraTarget - position);
        pitch = asin(direction.y);
        yaw = atan2(direction.z, direction.x);

        // TODO Inefficient because keep converting back-and-forth between degrees and radians
        pitch = glm::degrees(pitch);
        yaw = glm::degrees(yaw);

        UpdateCameraVectors();
        RecalculateMatrices();
    }
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 worldUp;

    float yaw = -90;
    float pitch = 0;

    float orthoWidth = 1, orthoHeight = 1;
    float fov = 45.0f;
    float aspectRatio;

    float near = 0.1f;
    float far = 1000;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix, invProjectionMatrix;

    bool isOrthographic = false;

    void UpdateCameraVectors();
    void RecalculateMatrices();
};