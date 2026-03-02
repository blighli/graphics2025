#pragma once
#include <glm/glm.hpp>

template <typename T>
static T clampT(T v, T lo, T hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

enum class CamMode { Orbit = 0, FPS = 1 };

struct OrbitCamera {
    glm::vec3 target{ 0,0,0 };
    float yaw = -90.0f;
    float pitch = 15.0f;
    float distance = 3.0f;

    glm::vec3 position() const;
    glm::mat4 view() const;
};

struct FPSCamera {
    glm::vec3 pos{ 0, 0.8f, 3.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;

    glm::vec3 forward() const;
    glm::mat4 view() const;
};

struct CameraSystem {
    CamMode mode = CamMode::Orbit;
    OrbitCamera orbit;
    FPSCamera fps;

    glm::vec3 camPos() const;
    glm::mat4 view() const;

    void reset();
};
