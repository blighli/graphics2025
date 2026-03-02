#ifndef PORTAL_H
#define PORTAL_H

#include <memory>
#include "portalRenderer.h"
#include "box.h"

namespace portal {

    const std::string frameVertShader = "portal.vs";
    const std::string frameFragShader = "portalFrame.fs";
    const std::string frameTexBlue    = "../texture/portalBlue.png";
    const std::string frameTexOrange  = "../texture/portalOrange.png";
    const std::string holeVertShader  = "portal.vs";
    const std::string holeFragShader  = "portalHole.fs";
    const std::string holeTex         = ""; // 留作空串

    class Portal {
    private:
        glm::vec3 faceDir;
        glm::vec3 pos;
        glm::vec3 upDir;
        glm::vec3 rightDir;
        bool hasBeenPlaced;
        Portal *pairedPortal;
        Renderable* frameRenderable;
        Renderable* holeRenderable;
        std::shared_ptr<Box> portalBox;     // 指向portal的判定碰撞区域
        std::shared_ptr<Box> portalableBox; // 指向portal的判定传送区域

    public:

        Portal(bool isFirst);
        ~Portal();

        void SetPair(Portal* pairedPortal);

        bool PlaceAt(glm::vec3 new_pos, glm::vec3 dir);
        bool HasBeenPlaced();
        bool IsLinkActive();

        glm::vec3 GetPosition();
        glm::vec3 GetFaceDir();
        glm::vec3 GetUpDir();
        Renderable* getFrameRenderable();
        Renderable* getHoleRenderable();
        Portal* GetPairedPortal();

        /**
         * @brief 转换View矩阵
         * 
         * @param view_matrix 要转换的view矩阵
         * @return glm::mat4 转换后的视图矩阵
         */
        glm::mat4 ConvertView( const glm::mat4& view_matrix );

        /**
         * @brief 转换玩家位置
         * 
         * @param point 当前位置
         * @return glm::vec3 转换后位置
         */
        glm::vec3 ConvertPointToOutPortal( glm::vec3 point );

        /**
         * @brief 转换摄像机方向
         * 
         * @param direction 当前方向
         * @param old_start_pos 旧的位置
         * @param new_start_pos 目标位置
         * @return glm::vec3 转换后的方向
         */
        glm::vec3 ConvertDirectionToOutPortal( glm::vec3 direction );

        // const btCollisionObject* GetAttachedCollisionObject();

        /// @brief 获取斜体视锥
        /// @param viewMat 
        /// @param projMat 
        /// @return glm::mat4 新的projMat
        glm::mat4 clipProjMat(glm::mat4 const &viewMat, glm::mat4 projMat);
    };

}

#endif