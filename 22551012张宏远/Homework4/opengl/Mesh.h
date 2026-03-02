#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

struct VertexPN {
    glm::vec3 pos;
    glm::vec3 nrm;
};

class Mesh {
public:
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;

    bool upload(const std::vector<VertexPN>& verts, const std::vector<uint32_t>& idx);
    void draw() const;
    void destroy();
};
