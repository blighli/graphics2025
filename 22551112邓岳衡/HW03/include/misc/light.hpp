#pragma once

#include "learn_vulkan.hpp"

namespace gameworld
{
    struct PointLight
    {
        alignas(16) glm::vec3 position = glm::vec3(0.0f);
        alignas(16) glm::vec3 color = glm::vec3(1.0f);
        alignas(4) float intensity = 1.0f;
    };

    struct DirectionalLight
    {
        alignas(16) glm::vec3 direction = glm::vec3(-1.0f, -1.0f, -1.0f);
        alignas(16) glm::vec3 color = glm::vec3(1.0f);
        alignas(4) float intensity = 1.0f;
    };

    struct SpotLight
    {
        alignas(16) glm::vec3 position = glm::vec3(0.0f);
        alignas(16) glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
        alignas(16) glm::vec3 color = glm::vec3(1.0f);
        alignas(4) float intensity = 1.0f;
        alignas(4) float cutOff = glm::cos(glm::radians(12.5f));
        alignas(4) float outerCutOff = glm::cos(glm::radians(15.0f));
    };
} // namespace gameworld
