#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 纹理加载库（需将stb_image.h放入项目目录）
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// -------------------------- 全局常量与变量 --------------------------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float SPHERE_RADIUS = 1.0f;

// 着色器程序ID
unsigned int shaderProgram;

// 球体VAO/VBO/EBO（共用一套顶点数据，通过model矩阵区分位置）
unsigned int sphereVAO, sphereVBO, sphereEBO;
int sphereIndexCount;

// 纹理ID（支持多纹理）
std::vector<unsigned int> textureIDs;

// 矩阵
glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 8.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 100.0f);

// 光照参数
glm::vec3 lightPos(2.0f, 3.0f, 4.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 8.0f); // 摄像机位置

// 球体数据结构（存储位置、纹理ID、名称）
struct Sphere {
    glm::vec3 position;
    unsigned int textureID;
    std::string name;
    Sphere(glm::vec3 pos, unsigned int texID, std::string n) : position(pos), textureID(texID), name(n) {}
};
std::vector<Sphere> spheres;

// -------------------------- 工具函数 --------------------------
// 检查着色器/程序编译链接错误
void checkShaderCompileError(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[512];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

// 生成球体顶点数据（经纬度细分）
void generateSphere(float radius, int sectors, int stacks) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // 生成顶点（坐标 + 法线 + 纹理坐标）
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = glm::pi<float>() * i / stacks;
        float y = radius * cos(stackAngle);
        float r = radius * sin(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = 2 * glm::pi<float>() * j / sectors;
            float x = r * cos(sectorAngle);
            float z = r * sin(sectorAngle);

            // 顶点坐标
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            // 法线（单位化）
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);
            // 纹理坐标
            vertices.push_back((float)j / sectors);
            vertices.push_back((float)i / stacks);
        }
    }

    // 生成索引
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = i * (sectors + 1) + j;
            int second = first + sectors + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    sphereIndexCount = indices.size();

    // 创建VAO/VBO/EBO
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 顶点属性配置
    // 位置属性 (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线属性 (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性 (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// 加载纹理
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 纹理参数设置
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载图片
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // 翻转Y轴（适配OpenGL纹理坐标）
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "ERROR::TEXTURE_LOAD_FAILED: " << path << std::endl;
        // 加载失败时使用默认红色纹理
        unsigned char defaultData[] = {255, 0, 0}; // 红色像素
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, defaultData);
    }
    stbi_image_free(data);
    return textureID;
}

// 射线与球体相交检测
bool raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
                        const glm::vec3& sphereCenter, float sphereRadius) {
    glm::vec3 oc = rayOrigin - sphereCenter;
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4 * a * c;
    return discriminant > 0;
}

// -------------------------- 回调函数 --------------------------
// 窗口大小调整
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(45.0f), (float)width/height, 0.1f, 100.0f);
}

// 键盘输入处理
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // 摄像机移动（W/A/S/D）
    float cameraSpeed = 0.05f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        viewPos += cameraSpeed * glm::normalize(glm::vec3(view[0][2], view[1][2], view[2][2]));
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        viewPos -= cameraSpeed * glm::normalize(glm::vec3(view[0][2], view[1][2], view[2][2]));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        viewPos -= glm::normalize(glm::cross(glm::vec3(view[0][2], view[1][2], view[2][2]), glm::vec3(0,1,0))) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        viewPos += glm::normalize(glm::cross(glm::vec3(view[0][2], view[1][2], view[2][2]), glm::vec3(0,1,0))) * cameraSpeed;
    
    // 更新视图矩阵
    view = glm::lookAt(viewPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// 鼠标点击回调（检测选中球体）
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // 屏幕坐标转NDC（归一化设备坐标）
        float x = (2.0f * xpos) / SCR_WIDTH - 1.0f;
        float y = 1.0f - (2.0f * ypos) / SCR_HEIGHT;
        float z = 1.0f;
        glm::vec3 rayNDC(x, y, z);

        // NDC转裁剪空间
        glm::vec4 rayClip = glm::vec4(rayNDC, 1.0f);

        // 裁剪空间转视图空间（逆投影矩阵）
        glm::vec4 rayEye = glm::inverse(projection) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

        // 视图空间转世界空间（逆视图矩阵）
        glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

        // 检测所有球体
        bool selected = false;
        for (const auto& sphere : spheres) {
            if (raySphereIntersect(viewPos, rayWorld, sphere.position, SPHERE_RADIUS)) {
                std::cout << "select: " << sphere.name << std::endl;
                selected = true;
                break;
            }
        }
        if (!selected) {
            std::cout << "noselected" << std::endl;
        }
    }
}

// -------------------------- 着色器源码 --------------------------
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// 光照参数
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// 纹理
uniform sampler2D texture1;

void main() {
    // 环境光
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 镜面反射
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // 纹理采样
    vec4 texColor = texture(texture1, TexCoord);

    // 最终颜色
    vec3 result = (ambient + diffuse + specular) * vec3(texColor);
    FragColor = vec4(result, texColor.a);
}
)";

// -------------------------- 主函数 --------------------------
int main() {
    // 1. 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Sphere (Light+Texture+Selection)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // 3. 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 4. 编译着色器程序
    // 顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompileError(vertexShader, "VERTEX");

    // 片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompileError(fragmentShader, "FRAGMENT");

    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShaderCompileError(shaderProgram, "PROGRAM");

    // 删除临时着色器
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 5. 初始化球体和纹理
    generateSphere(SPHERE_RADIUS, 36, 18); // 高细分球体（36经度，18纬度）
    
    // 加载纹理（替换为你的纹理路径，或使用默认红色纹理）
    textureIDs.push_back(loadTexture("red_texture.jpg"));  // 球体1纹理
    textureIDs.push_back(loadTexture("blue_texture.jpg")); // 球体2纹理
    textureIDs.push_back(loadTexture("green_texture.jpg"));// 球体3纹理

    // 创建多个球体
    spheres.emplace_back(glm::vec3(-2.5f, 0.0f, 0.0f), textureIDs[0], "redball");
    spheres.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), textureIDs[1], "blueball");
    spheres.emplace_back(glm::vec3(2.5f, 0.0f, 0.0f), textureIDs[2], "greenball");

    // 6. 开启深度测试（必须，否则3D物体渲染错误）
    glEnable(GL_DEPTH_TEST);

    // 7. 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 输入处理
        processInput(window);

        // 清除缓冲（颜色+深度）
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 使用着色器程序
        glUseProgram(shaderProgram);

        // 设置全局Uniform
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // 绘制所有球体
        glBindVertexArray(sphereVAO);
        for (const auto& sphere : spheres) {
            // 设置模型矩阵（位置）
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, sphere.position);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // 绑定纹理并绘制
            glBindTexture(GL_TEXTURE_2D, sphere.textureID);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        // 交换缓冲+轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 8. 释放资源
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    for (auto texID : textureIDs) glDeleteTextures(1, &texID);
    glDeleteProgram(shaderProgram);

    // 终止GLFW
    glfwTerminate();
    return 0;
}