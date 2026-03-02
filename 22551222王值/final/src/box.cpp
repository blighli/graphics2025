#include "box.h"

std::vector<std::shared_ptr<Box>> wallBoxList;
std::shared_ptr<Box> bluePortalBox;
std::shared_ptr<Box> bluePortalableBox;
std::shared_ptr<Box> orangePortalBox;
std::shared_ptr<Box> orangePortalableBox;
std::shared_ptr<Box> cameraBox;

Box::Box(glm::vec3 p1, glm::vec3 p2, BoxType type) {
    this->p1 = p1;
    this->p2 = p2;
    this->type = type;
}

Box::~Box() {}

bool Box::checkCollision(Box &b1, Box &b2) {
    bool xCollision = b1.getP1().x >= b2.getP2().x && b2.getP1().x >= b1.getP2().x;
    bool yCollision = b1.getP1().y >= b2.getP2().y && b2.getP1().y >= b1.getP2().y;
    bool zCollision = b1.getP1().z >= b2.getP2().z && b2.getP1().z >= b1.getP2().z;
    return xCollision && yCollision && zCollision;
}

bool Box::checkInside(Box &b_out, Box &b_in) {
    bool xIn = b_out.getP1().x >= b_in.getP1().x && b_out.getP2().x <= b_in.getP2().x;
    bool yIn = b_out.getP1().y >= b_in.getP1().y && b_out.getP2().y <= b_in.getP2().y;
    bool zIn = b_out.getP1().z >= b_in.getP1().z && b_out.getP2().z <= b_in.getP2().z;
    return xIn && yIn && zIn;
}

bool Box::checkPlaceAt(Box &b, glm::vec3 cameraPos, glm::vec3 dir, glm::vec3 &resPos, glm::vec3 &resNorm) {
    
    if (b.getType() == BoxType::PortalBox || b.getType() == BoxType::CameraBox) return false;

    float temp;
    glm::vec3 cross[6];
    // 计算六个可能的交点
    temp = (b.getP1().x - cameraPos.x) / dir.x;
    cross[0] = glm::vec3(b.getP1().x, cameraPos.y + temp * dir.y, cameraPos.z + temp * dir.z);
    temp = (b.getP2().x - cameraPos.x) / dir.x;
    cross[1] = glm::vec3(b.getP2().x, cameraPos.y + temp * dir.y, cameraPos.z + temp * dir.z);

    temp = (b.getP1().y - cameraPos.y) / dir.y;
    cross[2] = glm::vec3(cameraPos.x + temp * dir.x, b.getP1().y, cameraPos.z + temp * dir.z);
    temp = (b.getP2().y - cameraPos.y) / dir.y;
    cross[3] = glm::vec3(cameraPos.x + temp * dir.x, b.getP2().y, cameraPos.z + temp * dir.z);
    
    temp = (b.getP1().z - cameraPos.z) / dir.z;
    cross[4] = glm::vec3(cameraPos.x + temp * dir.x, cameraPos.y + temp * dir.y, b.getP1().z);
    temp = (b.getP2().z - cameraPos.z) / dir.z;
    cross[5] = glm::vec3(cameraPos.x + temp * dir.x, cameraPos.y + temp * dir.y, b.getP2().z);

    bool isCross = false;
    glm::vec3 trueCross;
    glm::vec3 trueNormal;
    for (int i = 0; i < 6; i++) {
        // 点要在平面上且与dir同向
        if (cross[i].x <= b.getP1().x && cross[i].x >= b.getP2().x &&
            cross[i].y <= b.getP1().y && cross[i].y >= b.getP2().y &&
            cross[i].z <= b.getP1().z && cross[i].z >= b.getP2().z &&
            glm::dot(cross[i] - cameraPos, dir) > 0
            ) {
            if (!isCross) {
                trueCross = cross[i];
                isCross = true;
            }
            else if (glm::length(trueCross - cameraPos) > glm::length(cross[i] - cameraPos)) {
                trueCross = cross[i];
            }
            else continue;
            float sign = i % 2 ? -1.f : 1.f;
            trueNormal = i < 2 ? glm::vec3(sign, 0.f, 0.f) : (i < 4 ? glm::vec3(0.f, sign, 0.f) : glm::vec3(0.f, 0.f, sign));
        }
    }
    if (isCross && glm::length(resPos - cameraPos) > glm::length(trueCross - cameraPos)) {
        resPos = trueCross;
        resNorm = trueNormal;
    }

    return isCross;
}

void Box::setP1(glm::vec3 p) {p1 = p;}
void Box::setP2(glm::vec3 p) {p2 = p;}
void Box::setP1(BoxType type) {this->type = type;}

glm::vec3 Box::getP1() {return p1;}
glm::vec3 Box::getP2() {return p2;}
BoxType Box::getType() {return type;}
