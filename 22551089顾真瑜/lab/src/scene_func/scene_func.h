#ifndef SCENEFUNC_H
#define SCENEFUNC_H

/// @warning: these secnes are not really a scene. just to store different scene code.

#include "../shader.h"
#include "../camera.h"
#include "../glMotion.h"
#include "../mesh.h"
#include "../model.h"
#include "../texture.h"
#include "../material.h"
#include "../light.h"

extern Camera camera;
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

class Scene{

public:

    Scene(){};
    virtual void renderFunc() = 0;

};

// Lab1
class Scene_Triangle : public Scene{

public:

    Scene_Triangle();
    ~Scene_Triangle();
    virtual void renderFunc() override;

private:

    Shader* triangle_shader;
    unsigned int VBO, VAO;
    float rotation;
};

// Lab2
class Scene_SolarSystem : public Scene{

public:

    Scene_SolarSystem();
    ~Scene_SolarSystem();
    virtual void renderFunc() override;

private:

    Shader* sun_shader, *obj_shader, *skybox_shader;
    // objs
    Sphere* earth, *moon, *sun;
    float earth_autorotation_rate, moon_autorotation_rate, sun_autorotation_rate;
    float earth_revolution_speed, moon_revolution_speed;
    float earth_orbit_radius, moon_orbit_radius;

    float earth_rotation_angle, moon_rotation_angle, sun_rotation_angle;
    float earth_revolution_angle, moon_revolution_angle;
};

// Lab3
class Scene_Nanosuit : public Scene{

public:

    Scene_Nanosuit();
    ~Scene_Nanosuit();
    virtual void renderFunc() override;

private:

    // Shaders
    Shader *obj_shader, *light_shader;

    // objects
    Model *mymodel;
    Cube *light_cube;

    // lights
    Light* direct_light;

};

#endif
