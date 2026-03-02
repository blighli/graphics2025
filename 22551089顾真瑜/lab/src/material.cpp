#include "material.h"
#include <string>
using std::string;

void PhongMaterial::setMaterial(Shader *shader) const
{
    shader->use();
    if(diffuse.x > 0.f)
        shader->setVec3(name + ".diffuse",  diffuse);
    if(specular.x > 0.f)
        shader->setVec3(name + ".specular", specular);

    shader->setFloat(name + ".shininess", shininess);

    // handle diffuse and specular textures
    for(unsigned int i = 0; i < textures.size(); i++){
        string tname = textures[i]->name;
        // bind tname to GL_TEXTURE_i
        shader->setInt(name + "." + tname, i);
    }
}

void PhongMaterial::activeTexture() const
{
    assert(textures.size() <= 16);
    for(unsigned int i = 0; i < textures.size(); i++){
        unsigned int tid = textures[i]->id;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, tid);
    }
}

void PhongMaterial::inactiveTexture() const
{
    assert(textures.size() <= 16);
    for(unsigned int i = 0; i < textures.size(); i++){
        unsigned int tid = textures[i]->id;
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
