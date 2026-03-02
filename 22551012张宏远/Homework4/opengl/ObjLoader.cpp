#include "ObjLoader.h"
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

static glm::vec3 safeNormalize(const glm::vec3& v) {
    float len = glm::length(v);
    if (len < 1e-8f) return glm::vec3(0, 1, 0);
    return v / len;
}

static void computeNormals(std::vector<VertexPN>& vertices, const std::vector<uint32_t>& indices) {
    std::vector<glm::vec3> acc(vertices.size(), glm::vec3(0));
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
        glm::vec3 a = vertices[ia].pos;
        glm::vec3 b = vertices[ib].pos;
        glm::vec3 c = vertices[ic].pos;
        glm::vec3 n = glm::cross(b - a, c - a);
        acc[ia] += n; acc[ib] += n; acc[ic] += n;
    }
    for (size_t i = 0; i < vertices.size(); ++i) vertices[i].nrm = safeNormalize(acc[i]);
}

bool loadOBJ_PN_Fast(const std::string& path, std::vector<VertexPN>& outVerts, std::vector<uint32_t>& outIdx) {
    std::cout << "▶ Load OBJ: " << path << "\n";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), nullptr, true);
    if (!warn.empty()) std::cout << "⚠ " << warn << "\n";
    if (!err.empty())  std::cerr << "❌ " << err << "\n";
    if (!ok) return false;

    size_t vCount = attrib.vertices.size() / 3;
    std::cout << "  v=" << vCount << " shapes=" << shapes.size() << "\n";

    outVerts.assign(vCount, VertexPN{});
    for (size_t i = 0; i < vCount; i++) {
        outVerts[i].pos = glm::vec3(attrib.vertices[3 * i + 0], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2]);
        outVerts[i].nrm = glm::vec3(0);
    }

    size_t totalIdx = 0;
    for (auto& s : shapes) totalIdx += s.mesh.indices.size();
    outIdx.clear();
    outIdx.reserve(totalIdx);

    for (auto& s : shapes) {
        for (auto& id : s.mesh.indices) {
            if (id.vertex_index >= 0) outIdx.push_back((uint32_t)id.vertex_index);
        }
    }

    computeNormals(outVerts, outIdx);
    std::cout << "  verts=" << outVerts.size() << " idx=" << outIdx.size() << "\n";
    return true;
}

void centerAndScalePN(std::vector<VertexPN>& verts, float targetSize) {
    if (verts.empty()) return;
    glm::vec3 mn(1e30f), mx(-1e30f);
    for (auto& v : verts) { mn = glm::min(mn, v.pos); mx = glm::max(mx, v.pos); }
    glm::vec3 center = (mn + mx) * 0.5f;
    glm::vec3 extent = (mx - mn);
    float maxE = std::max(extent.x, std::max(extent.y, extent.z));
    float s = (maxE > 1e-6f) ? (targetSize / maxE) : 1.0f;
    for (auto& v : verts) v.pos = (v.pos - center) * s;
}
