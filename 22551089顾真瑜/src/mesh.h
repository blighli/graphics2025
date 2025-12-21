#ifndef MESH_H
#define MESH_H
#define _USE_MATH_DEFINES

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "material.h"

#include <vector>
#include <cmath>
using std::vector;

struct Vertex{
    glm::vec3 pos;
    glm::vec2 uv;   // texture coordinate, if no texture, just ignore this, fill (0.f, 0.f, 0.f).
    glm::vec3 normal;
    // TODO: add more infomation

    Vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) {
        pos.x = x;
        pos.y = y;
        pos.z = z;
        uv.x = u;
        uv.y = v;
        normal.x = nx;
        normal.y = ny;
        normal.z = nz;
    }
    Vertex(glm::vec3 _pos, glm::vec2 _uv, glm::vec3 _normal){
        pos = _pos;
        uv = _uv;
        normal = _normal;
    }
};

class Mesh{

public:
    /// @brief constructor of Mesh
    /// @param vertices a vector of Vertex
    /// @param indices id of Vertex, for EBO
    /// @param material mesh's material
    Mesh(Shader* sh, const vector<Vertex>& vertices, const vector<unsigned int>& indices, Material* material);

    void Draw(const unsigned int vertex_num = 0, const unsigned int mesh_type = GL_TRIANGLES) const;

    Material* GetMaterial() const { return material; };

    /// @brief each mesh bind to a shader program
    Shader* shader;

protected:

    Mesh(){};   // never use

    vector<Vertex> vertices;
    vector<unsigned int> indices;   // for EBO
    Material* material;
    unsigned int VAO, VBO, EBO;

};

/// @brief a type of easy mesh
class Cube : public Mesh{

public:

    Cube(Shader* sh, Material* material);
};

/// @brief a type of easy mesh - Sphere
class Sphere : public Mesh {
public:
    /// @brief constructor of Sphere
    /// @param sh shader program
    /// @param material sphere's material
    /// @param radius sphere radius (default = 1.0f)
    /// @param sectorCount number of sectors (longitude) (default = 36)
    /// @param stackCount number of stacks (latitude) (default = 18)
    Sphere(Shader* sh, Material* material,
           float radius = 1.0f,
           int sectorCount = 36,
           int stackCount = 18);
};

#endif
