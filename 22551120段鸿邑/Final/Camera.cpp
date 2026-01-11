#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

glm::mat3 Camera::getRotation() const {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, { 0,1,0 }));
    glm::vec3 up = glm::cross(right, front);

    return glm::mat3(right, up, -front);
}

void Camera::processKeyboard(int key, float dt) {
    glm::vec3 forward = -getRotation()[2];
    glm::vec3 right = getRotation()[0];

    float v = speed * dt;

    if (key == GLFW_KEY_W) position += forward * v;
    if (key == GLFW_KEY_S) position -= forward * v;
    if (key == GLFW_KEY_A) position -= right * v;
    if (key == GLFW_KEY_D) position += right * v;
}

void Camera::processMouse(float dx, float dy) {
    yaw += dx * sensitivity;
    pitch += dy * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}
