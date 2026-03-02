#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

static glm::vec3 safeNormalize(const glm::vec3& v) {
    float len = glm::length(v);
    if (len < 1e-8f) return glm::vec3(0, 1, 0);
    return v / len;
}

glm::vec3 OrbitCamera::position() const {
    float cy = std::cos(glm::radians(yaw));
    float sy = std::sin(glm::radians(yaw));
    float cp = std::cos(glm::radians(pitch));
    float sp = std::sin(glm::radians(pitch));
    glm::vec3 dir(cy * cp, sp, sy * cp);
    return target - dir * distance;
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(position(), target, glm::vec3(0, 1, 0));
}

glm::vec3 FPSCamera::forward() const {
    float cy = std::cos(glm::radians(yaw));
    float sy = std::sin(glm::radians(yaw));
    float cp = std::cos(glm::radians(pitch));
    float sp = std::sin(glm::radians(pitch));
    return safeNormalize(glm::vec3(cy * cp, sp, sy * cp));
}

glm::mat4 FPSCamera::view() const {
    glm::vec3 f = forward();
    return glm::lookAt(pos, pos + f, glm::vec3(0, 1, 0));
}

glm::vec3 CameraSystem::camPos() const {
    return (mode == CamMode::Orbit) ? orbit.position() : fps.pos;
}

glm::mat4 CameraSystem::view() const {
    return (mode == CamMode::Orbit) ? orbit.view() : fps.view();
}

void CameraSystem::reset() {
    mode = CamMode::Orbit;
    orbit = OrbitCamera{};
    fps = FPSCamera{};
}
