#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "shader.h"     // 自定义的shader创建器
#include "stb_image.h"  // 材质相关
#include "glm/glm.hpp"  // 矩阵运算相关
#include "texture.hpp"  // 材质
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "camera.h"     // 摄像头相关
#include "bindObject.hpp"
#include <iostream>
#include <vector>
#include <cmath>

// 回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// 和球相关的常量
const GLfloat PI= 3.14159265358979323846f;
const int Y_SEGMENTS = 64;
const int X_SEGMENTS = 64;

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// 摄像机
Camera camera(glm::vec3(0.0f, 0.0f, 20.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float lastFrame = 0;
float deltaTime = 0;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                  // version 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                  // versino .3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // core mode

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SolarSys", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 创建着色器
    glEnable(GL_DEPTH_TEST);
    Shader sunShader("../src/sun.vs", "../src/sun.fs");
    Shader planetShader("../src/planet.vs", "../src/planet.gs", "../src/planet.fs");

    // 计算球的顶点坐标, 序号
    std::vector<float> sphereVertices;
    std::vector<int> sphereIndices;

    for (int y=0; y<=Y_SEGMENTS; y++) {
        for (int x=0; x<=X_SEGMENTS; x++) {
            float xSeg = (float)x / (float)X_SEGMENTS;
            float ySeg = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSeg * 2.0f * PI) * std::sin(ySeg * PI);
            float yPos = std::cos(ySeg * PI);
            float zPos = std::sin(xSeg * 2.0f * PI) * std::sin(ySeg * PI);
            sphereVertices.push_back(xPos);
            sphereVertices.push_back(yPos);
            sphereVertices.push_back(zPos);
            // 纹理坐标
            sphereVertices.push_back(xSeg);
            sphereVertices.push_back(ySeg);
        }
    }

    for (int i=0;i<Y_SEGMENTS;i++) {
        for (int j=0;j<X_SEGMENTS;j++) {
            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j+1);
            sphereIndices.push_back(i* (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j + 1);
        }
    }

    unsigned int VAO, VBO, EBO;
    bindVAO_EBO_texture(sphereVertices, sphereIndices, VAO, VBO, EBO);

    // 绑定纹理
    unsigned int texture;
    loadTexture(texture, "../textures/sun.jpg");
    unsigned int earthTexture;
    loadTexture(earthTexture, "../textures/earth.jpg");
    unsigned int moonTexture;
    loadTexture(moonTexture, "../textures/moon.jpg");
    // render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // input
        processInput(window);

        // 清空颜色缓冲
        glClearColor(0.01f, 0.01f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // bind
        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        
        // view, proj矩阵
        glm::mat4 proj  = glm::mat4(1.0f);
        proj  = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        // -----------------------------------------------------------------------
        // 绘制太阳
        sunShader.use();
        sunShader.setMat4("view", camera.GetViewMatrix());
        sunShader.setMat4("projection", proj);

        // 恒星在中心，保持静止
        glm::vec3 color_sun = glm::vec3(1, 1, 1);    // 白色
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 sun_pos = glm::vec3(0.0f, 0.0f, 0.0f);  // 中心位置
        model = glm::translate(model, sun_pos);
        model = glm::scale(model, glm::vec3(2.0, 2.0, 2.0));
        sunShader.setMat4("model", model);
        sunShader.setVec3("lightColor", color_sun);
        glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

        // -----------------------------------------------------------------------
        // 绘制行星
        planetShader.use();
        planetShader.setMat4("view", camera.GetViewMatrix());
        planetShader.setMat4("projection", proj);
        planetShader.setVec3("viewPos", camera.Position);
        planetShader.setVec3("lightPosA", sun_pos);
        planetShader.setVec3("lightPosB", sun_pos);
        planetShader.setVec3("lightColorA", color_sun);
        planetShader.setVec3("lightColorB", color_sun);

        glBindTexture(GL_TEXTURE_2D, earthTexture);

        // 行星1号：绕中心恒星公转
        double radius_planet = 6.0;
        double speed_planet = 0.3;
        glm::vec3 orbit_planet = glm::vec3(std::cos(glfwGetTime() * speed_planet) * radius_planet,
                                           0.0f,
                                           std::sin(glfwGetTime() * speed_planet) * radius_planet);
        model = glm::mat4(1.0f);
        model = glm::translate(model, orbit_planet);
        model = glm::scale(model, glm::vec3(0.7, 0.7, 0.7));
        planetShader.setMat4("model", model);
        planetShader.setVec3("objectColor", 1, 1, 1);  // 白色
        glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

        // 行星的卫星：绕行星公转
        glBindTexture(GL_TEXTURE_2D, moonTexture);
        double radius_satellite = 2.0;
        double speed_satellite = 1.0;
        glm::vec3 orbit_satellite = glm::vec3(std::cos(glfwGetTime() * speed_satellite) * radius_satellite,
                                              0.0f,
                                              std::sin(glfwGetTime() * speed_satellite) * radius_satellite);
        model = glm::mat4(1.0f);
        model = glm::translate(model, orbit_satellite);
        model = glm::translate(model, orbit_planet);
        model = glm::scale(model, glm::vec3(0.4, 0.4, 0.4));
        planetShader.setMat4("model", model);
        planetShader.setVec3("objectColor", 0.9, 0.9, 0.9);  // 白色
        glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    float xpos_f = static_cast<float>(xpos);
    float ypos_f = static_cast<float>(ypos);
    if (firstMouse) {
        lastX = xpos_f;
        lastY = ypos_f;
        firstMouse = false;
    }
    float xoffset = xpos_f - lastX;
    float yoffset = ypos_f - lastY;
    lastX = xpos_f;
    lastY = ypos_f;
    // 这里设置了一下反转y轴
    camera.ProcessMouseMovement(xoffset, -yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}