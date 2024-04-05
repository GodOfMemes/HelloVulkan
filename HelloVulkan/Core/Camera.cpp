#include "Camera.hpp"
#include <glm/matrix.hpp>

Camera::Camera(const glm::vec3& position, float aspectRatio, const glm::vec3& worldUp)
    : position(position), aspectRatio(aspectRatio), worldUp(worldUp)
{
    UpdateCameraVectors();
    RecalculateMatrices();
}

void Camera::UpdateCameraVectors()
{
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::RecalculateMatrices()
{
    viewMatrix = glm::lookAt(position, position + front, up);
    
    if(isOrthographic)
    {
        projectionMatrix = glm::ortho(-orthoWidth / 2, orthoWidth / 2, -orthoHeight / 2, orthoHeight / 2, near, far);
    }
    else 
    {
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, near,far);
    }
    projectionMatrix[1][1] *= -1;

    invProjectionMatrix = glm::inverse(projectionMatrix);
}
