#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
using std::string;
using std::vector;
using glm::vec3;

struct Texture;

class Material{

public:


    virtual void setMaterial(Shader* shader) const = 0;
    virtual void activeTexture() const = 0;
    virtual void inactiveTexture() const = 0;

protected:

    Material(){};   // never use

};

class PhongMaterial : public Material{

public:

    PhongMaterial(){};  // do nothing now

    PhongMaterial(string material_name, vec3 diffuse, vec3 specular, float shininess = 32.f){
        this->name = material_name;
        this->diffuse = diffuse;
        this->specular = specular;
        this->shininess = shininess;
    }

    PhongMaterial(string material_name, vector<Texture*> textures, float shininess = 32.f){
        this->name = material_name;
        this->textures = textures;
        this->shininess = shininess;

        // set diffuse and specular useless
        diffuse = vec3(-1.0f);
        specular = vec3(-1.0f);
    }

    ~PhongMaterial(){
        for(auto t : textures){
            delete t;
        }
    }

    virtual void setMaterial(Shader* shader) const override;
    virtual void activeTexture() const override;
    virtual void inactiveTexture() const override;  // HACK: use default black texture is better

private:
    string name;    // the material name in shader
    vec3 diffuse, specular;
    vector<Texture*> textures;
    float shininess;

};





#endif
