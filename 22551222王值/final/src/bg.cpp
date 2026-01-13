#include <memory>
#include "bg.h"
#include "glm/glm.hpp"  // 矩阵运算相关
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "box.h"

using namespace bg;

Bg::Bg(glm::vec3 pos, const std::string &obj_path, const std::string &texture_path)
{
    this->pos = pos;
    renderable = new Renderable(obj_path, vertShader, fragShader, texture_path);
    auto real_model = renderable->GetModel();
    real_model = glm::translate(real_model, pos);
    renderable->SetModel(real_model);

    glm::vec3 p1, p2;
    renderable->GetP1P2(p1, p2);
    // std::cout << p1.x << " " << p1.y << " " << p1.z << std::endl;
    // std::cout << p2.x << " " << p2.y << " " << p2.z << std::endl;
    std::shared_ptr<Box> b = std::make_shared<Box>(Box(p1, p2, BoxType::wallBox));
    wallBoxList.push_back(b);
    box = b;
}

Bg::Bg(glm::vec3 pos, const std::string &obj_path, const std::string &texture_path, glm::vec3 scale)
{
    this->pos = pos;
    renderable = new Renderable(obj_path, vertShader, fragShader, texture_path);
    auto real_model = renderable->GetModel();
    real_model = glm::translate(real_model, pos);
    real_model = glm::scale(real_model, scale);
    renderable->SetModel(real_model);

    glm::vec3 p1, p2;
    renderable->GetP1P2(p1, p2);
    // std::cout << p1.x << " " << p1.y << " " << p1.z << std::endl;
    // std::cout << p2.x << " " << p2.y << " " << p2.z << std::endl;
    std::shared_ptr<Box> b = std::make_shared<Box>(Box(p1, p2, BoxType::wallBox));
    wallBoxList.push_back(b);
    box = b;
}

Bg::~Bg() {

}

glm::vec3 Bg::GetPosition() {
    return pos;
}
void Bg::SetPosition(glm::vec3 delta){
    auto real_model = renderable->GetModel();
    real_model = glm::translate(real_model, delta);
    renderable->SetModel(real_model);
    this->pos += delta;
}
glm::vec3 Bg::GetSize()
{
    return size;
}

Renderable* Bg::getRenderable() {
    return renderable;
}
