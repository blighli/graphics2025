#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position{ 0.0f, 0.0f, 6.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 5.0f;
    float sensitivity = 0.1f;

    glm::mat3 getRotation() const;
    void processKeyboard(int key, float dt);
    void processMouse(float dx, float dy);
};
