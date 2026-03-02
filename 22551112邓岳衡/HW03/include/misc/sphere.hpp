#pragma once

#include "learn_vulkan.hpp"

#include <vector>

namespace gameworld
{
    struct SphereVertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    class Sphere
    {
    public:
        float radius = 1.0f;
        glm::vec3 center = glm::vec3(0.0f);

        std::vector<SphereVertex> vertices;
        std::vector<uint16_t> indices;

        Sphere() = default;
        Sphere(float radius, const glm::vec3 &center)
            : radius(radius), center(center)
        {
            generateMesh(radius, 36, 18);
        }

    private:
        // generate sphere mesh
        void generateMesh(float radius = 1.0f, int sectors = 36, int stacks = 18)
        {
            this->radius = radius;
            vertices.clear();
            indices.clear();

            for (int i = 0; i <= stacks; ++i)
            {
                float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stacks; // from pi/2 to -pi/2
                float xy = radius * cosf(stackAngle);                                    // r * cos(u)
                float z = radius * sinf(stackAngle);                                     // r * sin(u)

                for (int j = 0; j <= sectors; ++j)
                {
                    float sectorAngle = j * 2 * glm::pi<float>() / sectors; // from 0 to 2pi

                    SphereVertex vertex;
                    vertex.pos.x = xy * cosf(sectorAngle) + center.x; // r * cos(u) * cos(v)
                    vertex.pos.y = xy * sinf(sectorAngle) + center.y; // r * cos(u) * sin(v)
                    vertex.pos.z = z + center.z;                      // r * sin(u)

                    vertex.normal = glm::normalize(vertex.pos - center);
                    vertex.texCoord.x = static_cast<float>(j) / sectors;
                    vertex.texCoord.y = static_cast<float>(i) / stacks;

                    vertices.push_back(vertex);
                }
            }

            for (int i = 0; i < stacks; ++i)
            {
                int k1 = i * (sectors + 1); // beginning of current stack
                int k2 = k1 + sectors + 1;  // beginning of next stack

                for (int j = 0; j < sectors; ++j, ++k1, ++k2)
                {
                    if (i != 0)
                    {
                        indices.push_back(static_cast<uint16_t>(k1));
                        indices.push_back(static_cast<uint16_t>(k2));
                        indices.push_back(static_cast<uint16_t>(k1 + 1));
                    }

                    if (i != (stacks - 1))
                    {
                        indices.push_back(static_cast<uint16_t>(k1 + 1));
                        indices.push_back(static_cast<uint16_t>(k2));
                        indices.push_back(static_cast<uint16_t>(k2 + 1));
                    }
                }
            }
        }
    };
} // namespace world
