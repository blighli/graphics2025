#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "mesh.h"

#include <vector>
#include <string>
using std::vector;
using std::string;

// TODO: Optimize the model implementation to support more types of model imports
class Mesh;

class Model 
{

public:

    Model(Shader* shader, const char *path)
    {
        this->shader = shader;
        loadModel(path);
    }
    void Draw();   

    Shader* shader;

private:
    
    void loadModel(const string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    vector<Texture*> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName);
    
    vector<Texture*> textures_loaded;
    vector<Mesh> meshes;
    string directory;

};



#endif