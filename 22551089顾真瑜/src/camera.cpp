#include "camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
extern Camera camera;

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::Move(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;        
}

void Camera::Rotate(float d_pitch, float d_yaw, GLboolean constrainPitch)
{
    d_pitch *= MouseSensitivity;
    d_yaw *= MouseSensitivity;

    Pitch += d_pitch;
    Yaw += d_yaw;
    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::Zoom(float scale)
{
    ZoomScale -= (float)scale;
    if (ZoomScale < 1.0f)
        ZoomScale = 1.0f;
    if (ZoomScale > 45.0f)
        ZoomScale = 45.0f;
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up    = glm::normalize(glm::cross(Right, Front));
}
