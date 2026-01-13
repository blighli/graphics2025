#ifndef RENDER_ALL_H
#define RENDER_ALL_H

#include "bg.h"
#include "camera.h"
#include "lightRenderer.hpp"
#include "portal.h"

/// @brief 一定要初始化上下文再创建这个类的实例
class RenderAll {
  private:
    portal::Portal* portals[2];             // 传送门的指针
    portal::PortalRenderer* portalRenderer; // 传送门渲染器的指针

    bg::BgRenderer* bgRenderer; // 背景渲染器的指针

    int scr_w, scr_h; // 获得proj需要宽度和高度
    glm::mat4 mainProj = glm::mat4(1.0f);
    glm::mat4 mainView = glm::mat4(1.0f);

    Shader *depth, *shadow, *debug_quad, *sun;

  public:
    light::LightRenderer* lightRenderer; // 渲染光照阴影贴图

    std::vector<bg::Bg*> bgs;   // 背景的指针

    RenderAll(Camera& camera, int width, int height);
    ~RenderAll();

    /// @brief 渲染整个场景，在main函数中调用
    /// @param camera
    void RenderScene(Camera& camera);

    /// @brief 渲染阴影
    void RenderLight(glm::mat4 view_mat, glm::mat4 proj_mat, Camera& camera);

    /// @brief 渲染传送门的场景
    /// @param thePortal 当前虚拟摄像机在哪个传送门后，真实摄像机用null表示
    /// @param view_mat
    /// @param proj_mat
    /// @param recursionLevel 迭代的层数
    void RenderPortals(portal::Portal* thePortal, glm::mat4 view_mat, glm::mat4 proj_mat, int recursionLevel);

    /// @brief 渲染除了传送门外的场景
    /// @param view_mat
    /// @param proj_mat
    void RenderBaseScene(glm::mat4 view_mat, glm::mat4 proj_mat);

    /// @brief 获取传送门指针
    /// @param isBlue true：蓝色门 false：橙色门
    /// @return 传送门指针
    portal::Portal* getPortalPtr(bool isBlue);
};

#endif