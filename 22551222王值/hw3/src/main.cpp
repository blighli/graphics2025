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
#include <fstream>
#include <sstream>

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

// 轨道视角模式（按住Shift）
bool orbitalMode = false;
float orbitalRadius = 20.0f; // 固定球面半径
glm::vec3 orbitalCenter(0.0f, 0.0f, 0.0f); // 星系中心

// 简易OBJ加载器：仅解析 v, vt, f(v/vt/…)，输出顶点数组 [x,y,z,u,v]
static bool loadOBJVerticesUV(const std::string& path, std::vector<float>& outVertices) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "Failed to open OBJ: " << path << std::endl;
        return false;
    }
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::string line;
    while (std::getline(file, line)) {
        if (line.size() < 2) continue;
        if (line.rfind("v ", 0) == 0) {
            std::istringstream iss(line.substr(2));
            glm::vec3 p; iss >> p.x >> p.y >> p.z; positions.push_back(p);
        } else if (line.rfind("vt ", 0) == 0) {
            std::istringstream iss(line.substr(3));
            glm::vec2 t; iss >> t.x >> t.y; texcoords.push_back(t);
        } else if (line.rfind("f ", 0) == 0) {
            std::istringstream iss(line.substr(2));
            std::vector<std::pair<int,int>> face; // (v, vt)
            std::string tok;
            while (iss >> tok) {
                int vi = 0, ti = 0; int ni = 0;
                size_t p1 = tok.find('/');
                if (p1 == std::string::npos) { // only v
                    vi = std::stoi(tok);
                } else {
                    size_t p2 = tok.find('/', p1 + 1);
                    std::string sv = tok.substr(0, p1);
                    std::string svt = (p2 == std::string::npos) ? tok.substr(p1 + 1)
                                                                : tok.substr(p1 + 1, p2 - p1 - 1);
                    if (!sv.empty()) vi = std::stoi(sv);
                    if (!svt.empty()) ti = std::stoi(svt);
                }
                face.emplace_back(vi, ti);
            }
            // 三角扇展开
            for (size_t i = 2; i < face.size(); ++i) {
                size_t idx[3] = {0, i - 1, i};
                for (int k = 0; k < 3; ++k) {
                    int vi = face[idx[k]].first;
                    int ti = face[idx[k]].second;
                    if (vi < 0) vi = static_cast<int>(positions.size()) + vi + 1;
                    if (ti < 0) ti = static_cast<int>(texcoords.size()) + ti + 1;
                    const glm::vec3& p = positions[vi - 1];
                    glm::vec2 t = glm::vec2(0.0f);
                    if (ti >= 1 && ti <= static_cast<int>(texcoords.size())) t = texcoords[ti - 1];
                    outVertices.push_back(p.x);
                    outVertices.push_back(p.y);
                    outVertices.push_back(p.z);
                    outVertices.push_back(t.x);
                    outVertices.push_back(t.y);
                }
            }
        }
    }
    return !outVertices.empty();
}

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

    // 加载椅子模型（替换地球用）
    std::vector<float> chairVertices;
    bool chairOk = loadOBJVerticesUV("../models/chair.obj", chairVertices);
    unsigned int chairVAO = 0, chairVBO = 0;
    int chairVertexCount = 0;
    if (chairOk) {
        chairVertexCount = static_cast<int>(chairVertices.size() / 5);
        glGenVertexArrays(1, &chairVAO);
        glGenBuffers(1, &chairVBO);
        glBindVertexArray(chairVAO);
        glBindBuffer(GL_ARRAY_BUFFER, chairVBO);
        glBufferData(GL_ARRAY_BUFFER, chairVertices.size() * sizeof(float), chairVertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    } else {
        std::cout << "Warning: chair.obj load failed, fallback to sphere for Earth." << std::endl;
    }

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
        glm::mat4 view = camera.GetViewMatrix();
        if (orbitalMode) {
            // 固定在球面上，视角指向中心
            camera.Position = orbitalCenter - camera.Front * orbitalRadius;
            view = glm::lookAt(camera.Position, orbitalCenter, camera.Up);
        }
        
        // -----------------------------------------------------------------------
        // 绘制太阳（双星系统）
        sunShader.use();
        sunShader.setMat4("view", view);
        sunShader.setMat4("projection", proj);

        glm::vec3 color_sun = glm::vec3(1, 1, 1);
        glm::mat4 model = glm::mat4(1.0f);
        float t = static_cast<float>(glfwGetTime());
        float radius_binary = 6.0;
        float speed_binary = 0.5;
        glm::vec3 sunA_pos = glm::vec3(std::cos(t * speed_binary) * radius_binary,
                        0.0f,
                        std::sin(t * speed_binary) * radius_binary);
        glm::vec3 sunB_pos = -sunA_pos;

        // 太阳A
        model = glm::mat4(1.0f);
        model = glm::translate(model, sunA_pos);
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        sunShader.setMat4("model", model);
        sunShader.setVec3("lightColor", color_sun);
        glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

        // 太阳B
        model = glm::mat4(1.0f);
        model = glm::translate(model, sunB_pos);
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        sunShader.setMat4("model", model);
        sunShader.setVec3("lightColor", color_sun);
        glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

        // -----------------------------------------------------------------------
        // 绘制行星
        planetShader.use();
        planetShader.setMat4("view", view);
        planetShader.setMat4("projection", proj);
        planetShader.setVec3("viewPos", camera.Position);
        planetShader.setVec3("lightPosA", sunA_pos);
        planetShader.setVec3("lightPosB", sunB_pos);
        planetShader.setVec3("lightColorA", color_sun);
        planetShader.setVec3("lightColorB", color_sun);

        // 行星1号：绕太阳A公转
        glBindTexture(GL_TEXTURE_2D, earthTexture);
        double radius_planet = 6.0;
        double speed_planet = 1.0;
        glm::vec3 chair_offset = glm::vec3(0.0f, -0.5f, 0.0f); // 椅子模型偏移调整
        glm::vec3 planet_pos = sunA_pos + glm::vec3(
            std::cos(glfwGetTime() * speed_planet) * static_cast<float>(radius_planet),
            0.0f,
            std::sin(glfwGetTime() * speed_planet) * static_cast<float>(radius_planet)
        );
        model = glm::mat4(1.0f);
        model = glm::translate(model, planet_pos + chair_offset);
        model = glm::scale(model, glm::vec3(0.7, 0.7, 0.7));
        planetShader.setMat4("model", model);
        planetShader.setVec3("objectColor", 1, 1, 1);  // 白色
        if (chairOk) {
            glBindVertexArray(chairVAO);
            glDrawArrays(GL_TRIANGLES, 0, chairVertexCount);
            glBindVertexArray(0);
        } else {
            // 椅子加载失败则使用原球体
            glBindVertexArray(VAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);
        }

        // 行星的卫星：绕行星公转
        glBindTexture(GL_TEXTURE_2D, moonTexture);
        double radius_satellite = 2.0;
        double speed_satellite = 1.0;
        glm::vec3 orbit_satellite = glm::vec3(std::cos(glfwGetTime() * speed_satellite) * radius_satellite,
                                              0.0f,
                                              std::sin(glfwGetTime() * speed_satellite) * radius_satellite);
        model = glm::mat4(1.0f);
        model = glm::translate(model, orbit_satellite);
        model = glm::translate(model, planet_pos);
        model = glm::scale(model, glm::vec3(0.4, 0.4, 0.4));
        planetShader.setMat4("model", model);
        planetShader.setVec3("objectColor", 0.9, 0.9, 0.9);  // 白色
        // 卫星仍使用球体模型
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    if (chairVAO) glDeleteVertexArrays(1, &chairVAO);
    if (chairVBO) glDeleteBuffers(1, &chairVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    // Shift 切换到球面环绕视角（按住生效）
    orbitalMode = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                  (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (!orbitalMode) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
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