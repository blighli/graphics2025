#include "portal.h"
#include "glm/glm.hpp"  // 矩阵运算相关
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "box.h"

using namespace portal;

namespace {

    const float PORTAL_FRAME_WIDTH = 4.4f;  // 传送门框的宽度
    const float PORTAL_FRAME_HEIGHT = 6.2f; // 传送门框的高度
    const float PORTAL_GUT_WIDTH = 2.f;     // 传送门的半宽度
    const float PORTAL_GUT_HEIGHT = 3.f;    // 传送门的半高度

    std::vector<Vertex> get_portal_frame() {
        return {
            {{ -PORTAL_FRAME_WIDTH / 2.f, -PORTAL_FRAME_HEIGHT / 2.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 0.f }, { 0.f, 0.f, 1.f }},
            {{ -PORTAL_FRAME_WIDTH / 2.f,  PORTAL_FRAME_HEIGHT / 2.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 1.f }, { 0.f, 0.f, 1.f }},
            {{ PORTAL_FRAME_WIDTH / 2.f,  -PORTAL_FRAME_HEIGHT / 2.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 0.f }, { 0.f, 0.f, 1.f }},
            {{ PORTAL_FRAME_WIDTH / 2.f,  -PORTAL_FRAME_HEIGHT / 2.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 0.f }, { 0.f, 0.f, 1.f }},
            {{ -PORTAL_FRAME_WIDTH / 2.f,  PORTAL_FRAME_HEIGHT / 2.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 0.f, 1.f }, { 0.f, 0.f, 1.f }},
            {{ PORTAL_FRAME_WIDTH / 2.f,   PORTAL_FRAME_HEIGHT / 2.f, 0.f }, { 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f }, { 0.f, 0.f, 1.f }},
        };
    }

    std::vector<Vertex> get_portal_ellipse_hole( float radius_x, float radius_y ) {
        const glm::vec4 fakeColor = {0.0f, 0.0f, 0.0f, 0.0f};
        const glm::vec2 fakeUv = {0.0f, 0.0f};
        const glm::vec3 normal = {0.0f, 0.0f, 1.0f};
        const float PI = 3.14159265;
        const int numSide = 30;
        constexpr int numVert = numSide + 2; 
        std::vector<Vertex> holeVert;
        holeVert.reserve(numVert);

        holeVert.emplace_back(Vertex{{0.0f, 0.0f, 0.0f}, fakeColor, fakeUv, normal});   // emplace 性能好一些

        for (int i = 0; i < numVert; i++) {
            float rad = (float)(numVert - i) / (float)numSide * 2 * PI;
            holeVert.emplace_back( Vertex {
                {cos(rad) * radius_x, sin(rad) * radius_y, 0.0f},
                fakeColor, fakeUv, normal
            });
        }
        return holeVert;
    }

    float maxF(float a, float b) {
        return a > b ? a : b;
    }

    float minF(float a, float b) {
        return a < b ? a : b;
    }

}

Portal::Portal(bool isFirst)
    : faceDir(0.0f, 0.0f, 1.0f)
    , pos(0.0f, 0.0f, 0.0f)
    , upDir(0.0f, 1.0f, 0.0f)
    , rightDir(-1.0f, 0.0f, 0.0f)
    , hasBeenPlaced(false)
    , pairedPortal(nullptr)
{   
    frameRenderable = new Renderable(get_portal_frame(),
                                    frameVertShader,
                                    frameFragShader,
                                    isFirst ? frameTexBlue : frameTexOrange);
    holeRenderable = new Renderable(get_portal_ellipse_hole(PORTAL_GUT_WIDTH, PORTAL_GUT_HEIGHT),
                                    holeVertShader,
                                    holeFragShader,
                                    holeTex,
                                    GL_TRIANGLE_FAN);
    
    portalBox = std::make_shared<Box>(Box(glm::vec3(PORTAL_GUT_WIDTH, PORTAL_GUT_HEIGHT, PORTAL_GUT_WIDTH / 2), 
                                          glm::vec3(-PORTAL_GUT_WIDTH, -PORTAL_GUT_HEIGHT, -PORTAL_GUT_WIDTH / 2),
                                          BoxType::PortalBox));
    portalableBox = std::make_shared<Box>(Box(glm::vec3(PORTAL_GUT_WIDTH, PORTAL_GUT_HEIGHT, 0.f), 
                                          glm::vec3(-PORTAL_GUT_WIDTH, -PORTAL_GUT_HEIGHT, -PORTAL_GUT_WIDTH / 2),
                                          BoxType::PortalBox));
    if (isFirst) {
        bluePortalBox = portalBox;
        bluePortalableBox = portalableBox;
    }
    else {
        orangePortalBox = portalBox;
        orangePortalableBox = portalableBox;
    }
    
}

Portal::~Portal() {

}

void Portal::SetPair(Portal* pairedPortal) {
    this->pairedPortal = pairedPortal;
}

bool Portal::PlaceAt(glm::vec3 new_pos, glm::vec3 dir) {

    if (glm::length(new_pos - this->GetPairedPortal()->GetPosition()) <= PORTAL_GUT_HEIGHT * 1.5) {
        return false;
    }

    glm::vec3 rot_axis;

    if (glm::normalize(dir) == glm::vec3(0.f, 0.f, 1.f)) {
        rot_axis = this->upDir;
    } else if (glm::normalize(dir) == glm::vec3(0.f, 0.f, -1.f)) {
        rot_axis = -this->upDir;
    } else {
        rot_axis = glm::normalize(glm::cross(glm::vec3(0.f, 0.f, 1.f), dir));
    }

    float theta = glm::acos(glm::dot(glm::vec3(0.f, 0.f, 1.f), dir));

    frameRenderable->SetModel(std::move(glm::mat4(1.f)));
    frameRenderable->Translate(new_pos + dir * 0.1f);
    frameRenderable->Rotate(theta, rot_axis);

    holeRenderable->SetModel(std::move(glm::mat4(1.f)));
    holeRenderable->Translate(new_pos + dir * 0.05f);
    holeRenderable->Rotate(theta, rot_axis);

    // 移动碰撞盒子
    glm::mat4 tmpModel = glm::mat4(1.f);
    tmpModel = glm::translate(tmpModel, new_pos);
    tmpModel = glm::rotate(tmpModel, theta, rot_axis);

    glm::vec3 tmp1 = glm::vec3(PORTAL_GUT_WIDTH, PORTAL_GUT_HEIGHT, PORTAL_GUT_WIDTH / 2);
    glm::vec3 tmp2 = glm::vec3(-PORTAL_GUT_WIDTH, -PORTAL_GUT_HEIGHT, -PORTAL_GUT_WIDTH / 2);
    glm::vec4 newP1 = tmpModel * glm::vec4(tmp1, 1.f);
    glm::vec4 newP2 = tmpModel * glm::vec4(tmp2, 1.f);
    portalBox->setP1(glm::vec3(maxF(newP1.x, newP2.x), maxF(newP1.y, newP2.y), maxF(newP1.z, newP2.z)));
    portalBox->setP2(glm::vec3(minF(newP1.x, newP2.x), minF(newP1.y, newP2.y), minF(newP1.z, newP2.z)));

    tmp1 = glm::vec3(PORTAL_GUT_WIDTH, PORTAL_GUT_HEIGHT, 0.f);
    tmp2 = glm::vec3(-PORTAL_GUT_WIDTH, -PORTAL_GUT_HEIGHT, -PORTAL_GUT_WIDTH / 2);
    newP1 = tmpModel * glm::vec4(tmp1, 1.f);
    newP2 = tmpModel * glm::vec4(tmp2, 1.f);
    portalableBox->setP1(glm::vec3(maxF(newP1.x, newP2.x), maxF(newP1.y, newP2.y), maxF(newP1.z, newP2.z)));
    portalableBox->setP2(glm::vec3(minF(newP1.x, newP2.x), minF(newP1.y, newP2.y), minF(newP1.z, newP2.z)));

    // std::cout << "P1 " << newP1.x << " " << newP1.y << " " << newP1.z << std::endl;
    // std::cout << "P2 " << newP2.x << " " << newP2.y << " " << newP2.z << std::endl;

    pos = new_pos;
    faceDir = dir;

    return true;
}

bool Portal::HasBeenPlaced() {
    /// @todo
    return true;
}

bool Portal::IsLinkActive() {
    /// @todo
    if (this->pairedPortal == nullptr)
        return false;
    else 
        return true;
}

glm::vec3 Portal::GetPosition() {
    return pos;
}

glm::vec3 Portal::GetFaceDir() {
    return faceDir;
}

glm::vec3 Portal::GetUpDir() {
    return upDir;
}

Renderable* Portal::getFrameRenderable() {
    return frameRenderable;
}

Renderable* Portal::getHoleRenderable() {
    return holeRenderable;
}

Portal* Portal::GetPairedPortal() {
    return pairedPortal;
}

glm::mat4 Portal::ConvertView( const glm::mat4& view_matrix ) {
    return view_matrix * glm::rotate(holeRenderable->GetModel(), 
                                     glm::radians(180.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f))
                       * glm::inverse(pairedPortal->getHoleRenderable()->GetModel());
}

glm::vec3 Portal::ConvertPointToOutPortal( glm::vec3 point ) {
    return !pairedPortal ? glm::vec3(0.0f) : 
                           pairedPortal->getHoleRenderable()->GetModel()
                           * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.1f, 0.0f))
                           * glm::inverse(holeRenderable->GetModel())
                           * glm::vec4(std::move(point), 1.0f);
}

glm::vec3 Portal::ConvertDirectionToOutPortal( glm::vec3 direction )
{
    // 由于转换的是方向不是一个点，这里先找出从传送门A到原方向上的一个点
	// 然后把该点进行变换，得到新的该点后与传送门B的位置组成一个新的单位方向
	// 再乘以老方向的"长度“即可
    glm::vec3 old_start_pos = this->GetPosition() + 0.05f * this->GetFaceDir();
    glm::vec3 new_start_pos = this->pairedPortal->GetPosition() + 0.05f * this->pairedPortal->GetFaceDir();
    glm::vec3 target = old_start_pos + glm::normalize(direction);
    target = ConvertPointToOutPortal(std::move(target));
    return glm::normalize(target - new_start_pos) * glm::length(direction);
}


glm::mat4 Portal::clipProjMat(glm::mat4 const &viewMat, glm::mat4 projMat) {
    glm::vec3 normal = glm::normalize(faceDir);                     // 法线
    glm::vec3 rand_pos = pos + 0.05f * normal;                      // 平面上任意一点，这里用中心位置
    glm::vec4 clipPlane(normal, - glm::dot(normal, rand_pos));      // 用于表示平面的向量：（Nx，Ny，Nz，N·randPos）
    clipPlane = glm::transpose(glm::inverse(viewMat)) * clipPlane;  // 转换到view空间中，点乘view的逆矩阵的转置矩阵
    
    // 按照公式计算
    glm::vec4 Q_prime(glm::sign(clipPlane.x), glm::sign(clipPlane.y), 1.0f, 1.0f);
    glm::vec4 Q = glm::inverse(projMat) * Q_prime;
    glm::vec4 projMat_4(0.0f, 0.0f, -1.0f, 0.0f);   // projMat第四行（列）
    glm::vec4 projMat_3 = (-2.0f * Q.z)             // 新的第三行（列）
                          / glm::dot(clipPlane, Q)
                          * clipPlane
                          - projMat_4;

    // openGl 的矩阵是列主序的
    for (int i = 0; i < 4; i++)
        projMat[i][2] = projMat_3[i];

    return projMat;
}