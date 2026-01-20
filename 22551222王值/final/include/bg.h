#ifndef BG_H
#define BG_H

#include "bgRenderer.h"

namespace bg {

    const std::string vertShader = "bg.vs";
    const std::string fragShader = "bg.fs";

    class Bg {
    private:
        glm::vec3 pos;
        glm::vec3 size; // length, width, height;
        Renderable* renderable;

    public:
        std::shared_ptr<Box> box;
        Bg(glm::vec3 pos, const std::string &obj_path, const std::string &texture_path);
        Bg(glm::vec3 pos, const std::string &obj_path, const std::string &texture_path, glm::vec3 scale);
        ~Bg();

        glm::vec3 GetPosition();
        void SetPosition(glm::vec3 pos);
        glm::vec3 GetSize();
        Renderable* getRenderable();
    };

}

#endif