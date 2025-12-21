#ifndef SCENEFUNC_H
#define SCENEFUNC_H

/// @warning: these secnes are not really a scene. just to store different scene code.

#include "../shader.h"

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

class Scene{

public:

    Scene(){};
    virtual void renderFunc() = 0;

};

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



#endif
