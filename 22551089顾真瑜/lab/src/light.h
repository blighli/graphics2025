#ifndef LIGHT_H
#define LIGHT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

#include <string>
using std::string;
using glm::vec3;

class Light {

public:

    virtual ~Light() = default;
    virtual void applyLight(Shader* shader) const = 0;

protected:

    Light(){};  // never use

    string name;
    vec3 ambient, diffuse, specular;

};

class DirectLight : public Light{

public:

    DirectLight(string light_name, vec3 ambient, vec3 diffuse, vec3 specular, vec3 direction){
        this->name = light_name;
        this->ambient = ambient;
        this->diffuse = diffuse;
        this->specular = specular;
        this->direction = direction;
    }

    virtual ~DirectLight() = default;

    virtual void applyLight(Shader* shader) const override{
        shader->use();
        shader->setVec3(name + ".ambient", ambient);
        shader->setVec3(name + ".diffuse", diffuse);
        shader->setVec3(name + ".specular", specular);
        shader->setVec3(name + ".direction", direction);
    }

private:

    vec3 direction;

};

class PointLight : public Light{

public:

    PointLight(string light_name, vec3 ambient, vec3 diffuse, vec3 specular, vec3 position, float constant, float linear, float quadratic){
        this->name = light_name;
        this->ambient = ambient;
        this->diffuse = diffuse;
        this->specular = specular;
        this->position = position;
        this->constant = constant;
        this->linear = linear;
        this->quadratic = quadratic;
    }

    virtual ~PointLight() = default;
    
    virtual void applyLight(Shader* shader) const override{
        shader->use();
        shader->setVec3(name + ".ambient", ambient);
        shader->setVec3(name + ".diffuse", diffuse);
        shader->setVec3(name + ".specular", specular);
        shader->setVec3(name + ".position", position);
        shader->setFloat(name + ".constant", constant);
        shader->setFloat(name + ".linear", linear);
        shader->setFloat(name + ".quadratic", quadratic);
    }

private:

    vec3 position;
    float constant;
    float linear;
    float quadratic;

};

class SpotLight : public Light{

public:

    /// @param cutOff   the cosine of inside cut angle
    /// @param outerCutOff  the cosine of outside cut angle
    SpotLight(string light_name, vec3 ambient, vec3 diffuse, vec3 specular, vec3 direction, 
              vec3 position, float constant, float linear, float quadratic, float cutOff, float outerCutOff){
        this->name = light_name;
        this->ambient = ambient;
        this->diffuse = diffuse;
        this->specular = specular;
        this->direction = direction;
        this->position = position;
        this->constant = constant;
        this->linear = linear;
        this->quadratic = quadratic;
        this->cutOff = cutOff;
        this->outerCutOff = outerCutOff;
    }

    virtual ~SpotLight() = default;

    virtual void applyLight(Shader* shader) const override{
        shader->use();
        shader->setVec3(name + ".ambient", ambient);
        shader->setVec3(name + ".diffuse", diffuse);
        shader->setVec3(name + ".specular", specular);
        shader->setVec3(name + ".direction", direction);
        shader->setVec3(name + ".position", position);
        shader->setFloat(name + ".constant", constant);
        shader->setFloat(name + ".linear", linear);
        shader->setFloat(name + ".quadratic", quadratic);
        shader->setFloat(name + ".cutOff", cutOff);
        shader->setFloat(name + ".outerCutOff", outerCutOff);
    }

private:

    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
};

#endif