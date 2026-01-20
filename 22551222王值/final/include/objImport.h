#ifndef OBJIMPORT_H
#define OBJIMPORT_H

#include "glad/glad.h"

#include "GLFW/glfw3.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define INF 99999999.0f

struct Vertex {
    GLfloat x, y, z;
};

struct Normal {
    GLfloat nx, ny, nz;
};

struct TexCoord {
    GLfloat u, v;
};

struct Face {
    GLuint v1, v2, v3;
    GLuint vn1, vn2, vn3;
    GLuint vt1, vt2, vt3;
};

class ObjModel {
public:
    std::vector<Vertex> vertices;
    std::vector<Normal> normals;
    std::vector<TexCoord> texcoords;
    std::vector<Face> faces;
    glm::vec3 maxP;     // AABB碰撞盒子的顶点
    glm::vec3 minP;     // AABB碰撞盒子的顶点

    ObjModel(const std::string &filename) {
        std::ifstream in(filename, std::ios::in);
        if (!in) {
            std::cerr << "Cannot open " << filename << std::endl;
            exit(1);
        }

        maxP = glm::vec3(-INF, -INF, -INF);
        minP = glm::vec3(INF, INF, INF);
        std::string line;
        while (getline(in, line)) {
            if (line.substr(0,2) == "v ") {
                std::istringstream s(line.substr(2));
                Vertex v;
                s >> v.x; s >> v.y; s >> v.z;
                if (v.x > maxP.x) maxP.x = v.x;
                if (v.y > maxP.y) maxP.y = v.y;
                if (v.z > maxP.z) maxP.z = v.z;
                if (v.x < minP.x) minP.x = v.x;
                if (v.y < minP.y) minP.y = v.y;
                if (v.z < minP.z) minP.z = v.z;
                vertices.push_back(v);
            } else if (line.substr(0,3) == "vn ") {
                std::istringstream s(line.substr(3));
                Normal n;
                s >> n.nx; s >> n.ny; s >> n.nz;
                normals.push_back(n);
            } else if (line.substr(0,3) == "vt ") {
                std::istringstream s(line.substr(3));
                TexCoord t;
                s >> t.u; s >> t.v;
                texcoords.push_back(t);
            } else if (line.substr(0,2) == "f ") {
                std::replace(line.begin(), line.end(), '-', ' '); // to read the data like "f -578/-578/-578" succussfully
                std::replace(line.begin(), line.end(), '/', ' ');
                std::istringstream s(line.substr(2));
                Face f;
                s >> f.v1; s >> f.vt1; s >> f.vn1;
                s >> f.v2; s >> f.vt2; s >> f.vn2;
                s >> f.v3; s >> f.vt3; s >> f.vn3;
                f.v1--; f.vt1--; f.vn1--;
                f.v2--; f.vt2--; f.vn2--;
                f.v3--; f.vt3--; f.vn3--;
                faces.push_back(f);
            } else if (line[0] == '#') { ; }
        }
    }
};

#endif // OBJIMPORT_H
