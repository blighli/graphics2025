#ifndef BOX_H
#define BOX_H

#include <vector>
#include <memory>
#include "glm/glm.hpp"  // 矩阵运算相关
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

class Box;

enum BoxType{
    wallBox,
    PortalBox,
    CameraBox
};

extern std::vector<std::shared_ptr<Box>> wallBoxList;
extern std::shared_ptr<Box> bluePortalBox;
extern std::shared_ptr<Box> bluePortalableBox;
extern std::shared_ptr<Box> orangePortalBox;
extern std::shared_ptr<Box> orangePortalableBox;
extern std::shared_ptr<Box> cameraBox;

/// @brief 各边与轴平行的Box
class Box {
public:
    /// @brief Box构造函数
    /// @param p1 xyz均最大的点
    /// @param p2 xyz均最小的点
    /// @param type 
    Box(glm::vec3 p1, glm::vec3 p2, BoxType type);
    ~Box();

    static bool checkCollision(Box &b1, Box &b2);
    static bool checkInside(Box &b_out, Box &b_in);
    static bool checkPlaceAt(Box &b, glm::vec3 cameraPos, glm::vec3 dir, glm::vec3 &resPos, glm::vec3 &resNorm);

    void setP1(glm::vec3 p);
    void setP2(glm::vec3 p);
    void setP1(BoxType type);

    glm::vec3 getP1();
    glm::vec3 getP2();
    BoxType getType();

private:
    BoxType type;
    glm::vec3 p1;
    glm::vec3 p2;
};

#endif