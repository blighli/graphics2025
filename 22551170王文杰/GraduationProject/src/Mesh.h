#pragma once
#include "assimp/include/assimp/Importer.hpp"
#include "glm/glm.hpp"

#include "Shader.h"

#include <string>
#include <vector>

#include "Shape.h"

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct MeshTexture {
	unsigned int id;
	std::string type;
	aiString path;
};


class Mesh
{
public:
	std::vector<Vertex> m_Vertices;
	std::vector<unsigned int> m_Indices;
	std::vector<MeshTexture> m_Textures;


private:
	unsigned int m_VAO;
	unsigned int m_VBO;
	unsigned int m_IBO;

	std::vector<Triangle_encoded> triangles;
	std::vector<glm::vec2> textureCoords;

	void setupMesh();
	
public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures);
	~Mesh() {}

	void Draw(Shader shader);
	std::vector<Triangle_encoded> getTriangleData() { return triangles; }
	std::vector<glm::vec2> getTextureCoords() { return textureCoords; }

	void encodeTriangle(glm::vec3 pos);
};