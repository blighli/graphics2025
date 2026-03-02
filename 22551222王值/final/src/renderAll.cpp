#include "renderAll.h"
#include "bg.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "lightRenderer.hpp"
#include "shader.h"
#include <GL/gl.h>

namespace {
const int MAX_RECURSION_LEVEL = 4;
const int PORTAL_BLUE = 0;
const int PORTAL_ORANGE = 1;
const bool ISBLUE = true;
const bool ISORANGE = false;
} // namespace
extern bg::Bg *selectedThing;
RenderAll::RenderAll(Camera& camera, int width, int height) {
    scr_w = width;
    scr_h = height;
    mainView = camera.GetViewMatrix();
    mainProj = glm::perspective(glm::radians(camera.Zoom), (float)scr_h / (float)scr_w, 0.1f, 1000.0f);
    // 初始化着色器
    depth = new Shader("depth.vs", "depth.fs");
    shadow = new Shader("shadow.vs", "shadow.fs");
    debug_quad = new Shader("debug_quad.vs", "debug_quad.fs");
    sun = new Shader("sun.vs", "sun.fs");
    // 初始化阴影贴图
    lightRenderer = new light::LightRenderer(depth, shadow, debug_quad);
    // portal 初始位置设置
    portals[PORTAL_BLUE] = new portal::Portal(ISBLUE);
    portals[PORTAL_ORANGE] = new portal::Portal(ISORANGE);
    portals[PORTAL_BLUE]->SetPair(portals[PORTAL_ORANGE]);
    portals[PORTAL_ORANGE]->SetPair(portals[PORTAL_BLUE]);

    portals[PORTAL_BLUE]->PlaceAt(glm::vec3(600.f, 0.f, 0.f), glm::normalize(glm::vec3(-1.f, 0.f, 0.f)));
    portals[PORTAL_ORANGE]->PlaceAt(glm::vec3(-600.f, 0.f, 0.f), glm::normalize(glm::vec3(1.f, 0.f, 0.f)));

    portalRenderer = new portal::PortalRenderer(scr_w, scr_h);

    // background 初始位置、obj、纹理图设置
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/ground2.obj", "../texture/tile.jpg"));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/walla.obj", "../texture/wall.png"));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/wallb.obj", "../texture/wall.png"));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/wallc.obj", "../texture/wall.png"));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/walld.obj", "../texture/wall.png"));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/walle.obj", "../texture/wall.png"));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -10.f, 0.f), "../models/wallf.obj", "../texture/wall.png"));

    bgs.push_back(new bg::Bg(glm::vec3(-13.f, -10.f, 0.f), "../models/chair.obj", "../texture/texture.jpg"));
    bgs.push_back(new bg::Bg(glm::vec3(-13.f, -10.f, 0.f), "../models/desk.obj", "../texture/texture.jpg"));
    bgs.push_back(new bg::Bg(glm::vec3(4.f, -7.f, -6.f), "../models/cube.obj", "../texture/texture.jpg", glm::vec3(2.f, 2.f, 2.f)));
    bgs.push_back(new bg::Bg(glm::vec3(2.f, -5.f, -6.f), "../models/cube.obj", "../texture/texture.jpg", glm::vec3(2.f, 2.f, 2.f)));
    bgs.push_back(new bg::Bg(glm::vec3(0.f, -7.f, -6.f), "../models/cube.obj", "../texture/texture.jpg", glm::vec3(2.f, 2.f, 2.f)));
    bgs.push_back(new bg::Bg(glm::vec3(2.f, 0.f, -6.f), "../models/ball.obj", "../texture/earth.jpg", glm::vec3(2.f, 2.f, 2.f)));
    bgs.push_back(new bg::Bg(lightRenderer->getLightPosition(), "../models/ball.obj", "../texture/sun.jpg", glm::vec3(5.f, 5.f, 5.f)));
    
    bgRenderer = new bg::BgRenderer(scr_w, scr_h);
}

RenderAll::~RenderAll() {
    delete (portals[PORTAL_BLUE]);
    delete (portals[PORTAL_ORANGE]);
    delete (portalRenderer);
    for (auto p : bgs) { delete (p); }
    delete (bgRenderer);
    delete lightRenderer;
}

void RenderAll::RenderScene(Camera& camera) {
    mainView = camera.GetViewMatrix();
    mainProj = glm::perspective(glm::radians(camera.Zoom), (float)scr_w / (float)scr_h, 0.1f, 100.0f);
    RenderLight(mainView, mainProj, camera);
    if (portals[PORTAL_BLUE]->IsLinkActive()) {
        RenderPortals(nullptr, mainView, mainProj, 0);
    } else {
        RenderBaseScene(mainView, mainProj);
    }
}

void RenderAll::RenderLight(glm::mat4 view_mat, glm::mat4 proj_mat, Camera& camera) {
    glCullFace(GL_FRONT);  // 解决阴影悬浮
    lightRenderer->renderDepth();
    for (auto bg : bgs) { bg->getRenderable()->SetShader(depth); }
    // bgs.back()->getRenderable()->SetShader(shadow);
    bgs.back()->getRenderable()->SetShader(sun);
    RenderBaseScene(view_mat, proj_mat);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // reset viewport
    glViewport(0, 0, 1200, 800);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_BACK);
    shadow->use();
    shadow->setMat4("proj", proj_mat);
    shadow->setMat4("view", view_mat);
    shadow->setVec3("viewPos", camera.Position);
    lightRenderer->renderShadow();
    for (auto bg : bgs) { bg->getRenderable()->SetShader(shadow); }
    bgs.back()->getRenderable()->SetShader(sun);
    // lightRenderer->renderQuad();  // used for debug
}

void RenderAll::RenderPortals(portal::Portal* thePortal, glm::mat4 view_mat, glm::mat4 proj_mat, int recursionLevel) {
    for (auto p : portals) {

        if (thePortal && p == thePortal) continue; // 除了第一次，剩下的递归只渲染一个传送门

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // 关闭颜色和深度缓存写入
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_STENCIL_TEST); // 开启模板测试
        glStencilFunc(GL_NOTEQUAL, recursionLevel,
                      0xFF); // 像素值不等于迭代级数时通过测试，也就是让范围限定在所有符合本层的像素
        glStencilOp(GL_INCR, GL_KEEP, GL_KEEP); // 不通过模板测试自增1，其他的不变
        glStencilMask(0xFF);                    // 表示每个像素8位的模板值都可用

        portalRenderer->setViewMat(view_mat);
        portalRenderer->setProjMat(proj_mat);
        portalRenderer->RenderPortal(p->getHoleRenderable());

        glm::mat4 new_view = p->ConvertView(view_mat);
        glm::mat4 new_proj = p->GetPairedPortal()->clipProjMat(new_view, proj_mat); // 裁切近投影面

        if (recursionLevel == MAX_RECURSION_LEVEL) {
            // 最大递归层数
            // 允许颜色和深度写入
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);

            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            glEnable(GL_STENCIL_TEST); // 开启模板测试，确保我们只在传送门内绘制
            glStencilMask(0x00);       // 禁止写入
            glStencilFunc(GL_EQUAL, recursionLevel + 1, 0xff); // 只对模板值满足条件的部分绘制

            RenderBaseScene(new_view, new_proj);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            portalRenderer->setViewMat(new_view);
            portalRenderer->setProjMat(new_proj);
            portalRenderer->RenderPortal(thePortal->GetPairedPortal()->getFrameRenderable());
            glDisable(GL_BLEND);
        } else {
            RenderPortals(p->GetPairedPortal(), new_view, new_proj, recursionLevel + 1);
        }

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);

        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);

        glStencilFunc(GL_NOTEQUAL, recursionLevel + 1, 0xFF); // 递归渲染的传送门内部会不通过这个模板测试
        glStencilOp(GL_DECR, GL_KEEP, GL_KEEP);               // 将模板值退回本层

        portalRenderer->setViewMat(view_mat);
        portalRenderer->setProjMat(proj_mat);
        for (auto p2 : portals) { portalRenderer->RenderPortal(p2->getHoleRenderable()); }
    }

    glDisable(GL_STENCIL_TEST);                          // 关闭模板测试
    glStencilMask(0x00);                                 // 关闭模板测试
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // 关闭颜色写入

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glDepthFunc(GL_ALWAYS);
    glClear(GL_DEPTH_BUFFER_BIT);

    portalRenderer->setViewMat(view_mat); // 将传送门的窗口写入到深度缓存
    portalRenderer->setProjMat(proj_mat);
    if (thePortal != nullptr) portalRenderer->RenderPortal(thePortal->GetPairedPortal()->getHoleRenderable());
    else {
        for (auto p : portals) { portalRenderer->RenderPortal(p->getHoleRenderable()); }
    }

    glDepthFunc(GL_LESS); // 将深度测试设回默认

    glEnable(GL_STENCIL_TEST); // 开启模板测试
    glStencilMask(0x00);
    glStencilFunc(GL_LEQUAL, recursionLevel, 0xff);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    RenderBaseScene(view_mat, proj_mat);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (thePortal != nullptr) {
        portalRenderer->setViewMat(view_mat);
        portalRenderer->setProjMat(proj_mat);
        portalRenderer->RenderPortal(thePortal->GetPairedPortal()->getFrameRenderable());
    } else {
        for (auto p : portals) {
            if (p->HasBeenPlaced()) {
                portalRenderer->RenderPortal(p->getFrameRenderable());
            }
        }
    }
    glDisable(GL_BLEND);
}

void RenderAll::RenderBaseScene(glm::mat4 view_mat, glm::mat4 proj_mat) {
    /// @todo
    bgRenderer->setViewMat(view_mat);
    bgRenderer->setProjMat(proj_mat);

    // render background
    for (auto p : bgs) {
        bgRenderer->RenderBg(p->getRenderable());
    }
}

portal::Portal* RenderAll::getPortalPtr(bool isBlue) {
    return isBlue ? portals[PORTAL_BLUE] : portals[PORTAL_ORANGE];
}