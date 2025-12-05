#include "Model.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#include "stb_image/stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "Shape.h"
#include "encoded/Triangle.h"

using namespace glm;

Model::Model(const std::string& filePath)
{
    loadModel(filePath);
}

void Model::Draw(Shader& shader)
{
    for (auto& mesh : m_Meshes)
        mesh.Draw(shader);
}

void Model::loadModel(const std::string path)
{
    Assimp::Importer importer;
    //ReadFile(path, 后期处理(不是三角形的变成三角形|y轴翻转))
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    m_Directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // 处理节点所有的网格（如果有的话）
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_Meshes.push_back(processMesh(mesh, scene));
    }
    // 接下来对它的子节点重复这一过程
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<MeshTexture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // TODO: caculate the vertex pos、normal and texcoord
        vertex.Position = glm::vec3(mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);

        vertex.Normal = glm::vec3(mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z);

        if (mesh->mTextureCoords[0])
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // TODO: caculate indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // TODO: caculate material
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<MeshTexture> diffuseMaps = loadMaterialTextures(material,
            aiTextureType_DIFFUSE, "texture_diffuse");

        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<MeshTexture> specularMaps = loadMaterialTextures(material,
            aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<MeshTexture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<MeshTexture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;

        // aiString 没有实现哈希散列函数 不能直接使用
        if (m_Textures_loaded.find(str.C_Str()) != m_Textures_loaded.end())
            continue;

        MeshTexture texture;
        texture.id = TextureFromFile(str.C_Str(), m_Directory);
        texture.type = typeName;
        texture.path = str;
        textures.push_back(texture);
        m_Textures_loaded.emplace(str.C_Str());
    }
    return textures;
}

unsigned int TextureFromFile(const std::string& path, const std::string& directory, bool gamma)
{
    std::string filename = path;
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void Model::encodedData(glm::vec3 pos)
{
    if (!triangles.empty())
        triangles.clear();

    std::string filename;
    unsigned char* data = nullptr;
    int width, height, nrComponents;

    for (auto& m : m_Meshes)
    {
        m.encodeTriangle(pos);
        auto TriangleData = m.getTriangleData();
        auto TexData = m.getTextureCoords();

        if (m.m_Textures.size() > 0) {
            filename = m.m_Textures[0].path.C_Str();
            filename = m_Directory + '/' + filename;
            //std::cout << "texture path：" << filename << std::endl;
            data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        }

        int n = TriangleData.size();

        //std::cout << "UV: (" << TexData[0].x << "," << TexData[0].y << ")" << std::endl;

        for (int i = 0; i < n; i++)
        {
            auto& t = TriangleData[i];

            if (data != nullptr)
            {
                auto& uv = TexData[i];
                t.baseColor = getTexColor(data, uv, width, height, nrComponents);
            }
            triangles.push_back(t);
        }
    }
}

glm::vec3 Model::getTexColor(const unsigned char* data, glm::vec2 uv, int w, int h, int nr)
{
    glm::vec3 res = glm::vec3(1.0);

    int idx = (int)(uv.x * h + 1) + (int)(uv.y * h + 1) * w;

    res.r = data[nr * idx] / 255.0;
    res.g = data[nr * idx + 1] / 255.0;
    res.b = data[nr * idx + 2] / 255.0;

    return res;
}

void Model::changeMaterial(Material& m)
{
    for (auto& t : triangles)
    {
        t.param3 = glm::vec3(m.sheen, m.sheenTine, 0.1f);
    }
}