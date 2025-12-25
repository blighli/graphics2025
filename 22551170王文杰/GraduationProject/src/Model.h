#pragma once
#include "Mesh.h"
#include "Shader.h"
#include "Shape.h"

#include <vector>
#include <string>
#include <unordered_set>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class Model: public shape
{
private:
	std::vector<Mesh> m_Meshes;
	std::string m_Directory;

	std::unordered_set<std::string> m_Textures_loaded;

	std::vector<Triangle_encoded> triangles;

public:
	Model(const std::string& filePath);
	~Model() {}

	HitResult intersect(Ray ray) { return HitResult(); }
	void Draw(Shader& shader);
	std::vector<Triangle_encoded> getCodedData() { return triangles; }
	void setEmissive(glm::vec3 emissive) {}
	void encodedData(glm::vec3 pos = glm::vec3(0));

	void changeMaterial(Material& m);

private:
	void loadModel(const std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<MeshTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

	glm::vec3 getTexColor(const unsigned char* data, glm::vec2 uv, int w, int h, int nr);
};

unsigned int TextureFromFile(const std::string& path, const std::string& directory, bool gamma = false);
