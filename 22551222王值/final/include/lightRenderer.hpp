#pragma once

#include "glm/fwd.hpp"
#include "shader.h"

namespace light {

class LightRenderer {
  private:
    int shadow_width = 8192, shadow_height = 8192;
    float near_plane = -100.0f, far_plane = 100.0f;
    GLuint depthMapFBO;
    GLuint depthMap;
    Shader *depth, *shadow, *quad;
    glm::mat4 lightProjection, lightView, lightSpaceMatrix;

  public:
    glm::vec3 lightPosition = glm::vec3{-20.0f, 40.0f, 20.0f};

    LightRenderer(Shader* depth, Shader* shadow, Shader* quad) : depth(depth), shadow(shadow), quad(quad) {
        // Setup some OpenGL options
        glEnable(GL_DEPTH_TEST);
        // - Create depth texture
        glGenTextures(1, &depthMap);
        glGenFramebuffers(1, &depthMapFBO);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_width, shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                     NULL);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // 我们宁可让所有超出深度贴图的坐标的深度范围是1.0，这样超出的坐标将永远不在阴影之中。我们可以储存一个边框颜色，然后把深度贴图的纹理环绕选项设置为GL_CLAMP_TO_BORDER：
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // attach depth texture as FBO's depth buffer
        shadow->use();
        shadow->setInt("diffuseTexture", 0);
        shadow->setInt("shadowMap", 1);
        quad->use();
        quad->setInt("depthMap", 0);
    }
    ~LightRenderer() = default;

    glm::vec3 getLightPosition() { return lightPosition; }

    void renderDepth() {
        // 1. render depth of scene to texture (from light's perspective)
        glEnable(GL_DEPTH);

        // lightPosition = glm::vec3{-2.0f, 4.0f, -1.0f};
        lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPosition, glm::vec3(0.0f, -10.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;
        depth->use();
        depth->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, shadow_width, shadow_height);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void renderShadow() {
        // set light uniforms
        shadow->use();
        shadow->setVec3("lightPos", lightPosition);
        shadow->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
    }

    void renderQuad() {
        // renderQuad() renders a 1x1 XY quad in NDC
        // -----------------------------------------
        static unsigned int quadVAO = 0;
        static unsigned int quadVBO;
        // render Depth map to quad for visual debugging
        // ---------------------------------------------
        quad->use();
        quad->setFloat("near_plane", near_plane);
        quad->setFloat("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        if (quadVAO == 0) {
            float quadVertices[] = {
                // positions        // texture Coords
                -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
            };
            // setup plane VAO
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        }
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
};

} // namespace light