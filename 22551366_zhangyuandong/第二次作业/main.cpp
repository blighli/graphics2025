#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <string>

// 纹理加载库
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// 窗口尺寸
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// 物理常量（精准匹配）
const float SUN_RADIUS = 3.0f;
const float EARTH_RADIUS = 1.0f;
const float MOON_RADIUS = 0.3f;
const float EARTH_ORBIT_RADIUS = 12.0f;
const float MOON_ORBIT_RADIUS = 2.5f;
const float SHADOW_BIAS = 0.01f; // 阴影偏移（防止自遮挡）

// 着色器程序ID
unsigned int shaderProgram;

// 纹理ID
unsigned int sunTexture, earthTexture, moonTexture;

// 球体顶点数据
std::vector<float> sphereVertices;
unsigned int sphereVAO, sphereVBO, sphereEBO;
int sphereIndexCount = 0;

// 天体位置（全局更新）
glm::vec3 sunPos = glm::vec3(0.0f);
glm::vec3 earthPos = glm::vec3(0.0f);
glm::vec3 moonPos = glm::vec3(0.0f);

// ===================== CPU端补充：射线-球体相交检测函数（修复编译错误） =====================
bool raySphereIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius, float& t0, float& t1) {
    glm::vec3 oc = rayOrigin - sphereCenter;
    float a = glm::dot(rayDir, rayDir);
    float b = 2.0f * glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) return false;
    
    float sqrtD = sqrt(discriminant);
    t0 = (-b - sqrtD) / (2.0f * a);
    t1 = (-b + sqrtD) / (2.0f * a);
    return true;
}

// ===================== 核心：精准阴影着色器 =====================
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    // 输出到片段着色器
    out vec2 TexCoord;
    out vec3 WorldPos;  // 世界空间坐标（精准）
    out vec3 Normal;    // 世界空间法线

    void main()
    {
        WorldPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aPos; // 球体法线=顶点方向
        TexCoord = aTexCoord;
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;
    in vec3 WorldPos;  // 像素的世界空间精准位置
    in vec3 Normal;

    // 全局参数（精准传递）
    uniform sampler2D texture1;
    uniform int objectType;      // 0=太阳, 1=地球, 2=月亮
    uniform vec3 sunPos;         // 太阳精准位置
    uniform vec3 earthPos;       // 地球精准位置
    uniform vec3 moonPos;        // 月亮精准位置
    uniform float sunRadius;     // 太阳半径
    uniform float earthRadius;   // 地球半径
    uniform float moonRadius;    // 月亮半径
    uniform float shadowBias;    // 阴影偏移

    // ===================== 精准射线-球体相交检测（GLSL版本） =====================
    bool raySphereIntersect(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius, out float t0, out float t1) {
        vec3 oc = rayOrigin - sphereCenter;
        float a = dot(rayDir, rayDir);
        float b = 2.0 * dot(oc, rayDir);
        float c = dot(oc, oc) - sphereRadius * sphereRadius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return false;
        
        float sqrtD = sqrt(discriminant);
        t0 = (-b - sqrtD) / (2.0 * a);
        t1 = (-b + sqrtD) / (2.0 * a);
        return true;
    }

    // ===================== 精准阴影检测 =====================
    bool isInPreciseShadow(vec3 fragPos, vec3 lightPos, vec3 occluderPos, float occluderRadius) {
        // 1. 构建从像素到光源的射线（反向：光源→像素，用于检测遮挡）
        vec3 rayDir = normalize(fragPos - lightPos);
        vec3 rayOrigin = lightPos + rayDir * shadowBias; // 偏移避免自遮挡
        
        // 2. 检测射线是否与遮挡物相交
        float t0, t1;
        if (raySphereIntersect(rayOrigin, rayDir, occluderPos, occluderRadius, t0, t1)) {
            // 3. 相交条件：遮挡物在光源和像素之间（t0 < 像素距离 且 t1 > 0）
            float fragDist = length(fragPos - lightPos);
            return (t0 < fragDist - shadowBias) && (t1 > 0.0);
        }
        return false;
    }

    void main() {
        vec4 texColor = texture(texture1, TexCoord);
        
        // 太阳自发光：无阴影，直接输出
        if (objectType == 0) {
            FragColor = texColor;
            return;
        }

        // ===================== 基础光照（精准漫反射） =====================
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(sunPos - WorldPos);
        float ambient = 0.15; // 环境光（避免纯黑）
        float diffuse = max(dot(norm, lightDir), 0.0);
        float totalLight = ambient + diffuse;

        // ===================== 精准阴影判断 =====================
        bool inShadow = false;

        // 地球的阴影：检测月亮是否遮挡太阳→地球的光
        if (objectType == 1) {
            inShadow = isInPreciseShadow(WorldPos, sunPos, moonPos, moonRadius);
        }
        // 月亮的阴影：检测地球是否遮挡太阳→月亮的光
        else if (objectType == 2) {
            inShadow = isInPreciseShadow(WorldPos, sunPos, earthPos, earthRadius);
        }

        // ===================== 最终颜色（精准阴影应用） =====================
        if (inShadow) {
            // 完全遮挡：仅保留环境光
            FragColor = texColor * ambient;
        } else {
            // 无遮挡：正常光照
            FragColor = texColor * totalLight;
        }
        FragColor.a = 1.0;
    }
)";

// 生成高精度球体（64*64细分，保证像素级精准）
void generateHighPrecisionSphere(float radius, int sectors = 64, int stacks = 64) {
    sphereVertices.clear();
    std::vector<unsigned int> indices;

    // 生成顶点（位置 + 纹理坐标）
    for (int i = 0; i <= stacks; i++) {
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stacks;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; j++) {
            float sectorAngle = j * 2 * glm::pi<float>() / sectors;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
            sphereVertices.push_back((float)j / sectors);
            sphereVertices.push_back((float)i / stacks);
        }
    }

    // 生成索引（三角面）
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < sectors; j++) {
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

    // 创建VAO/VBO/EBO（精准内存管理）
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 位置属性（精准布局）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// 加载纹理（保持原始精度）
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    // 加载原始精度纹理，不缩放
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 纹理过滤（精准采样）
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        stbi_image_free(data);
    } else {
        std::cerr << "纹理加载失败: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// 编译着色器（保留错误日志）
void compileShaders() {
    // 顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "顶点着色器编译失败:\n" << infoLog << std::endl;
    }

    // 片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "片段着色器编译失败:\n" << infoLog << std::endl;
    }

    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "着色器链接失败:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// 窗口回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 输入处理
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// 精准射线-球体碰撞检测（点击交互）
bool preciseRaySphereHit(double mouseX, double mouseY, GLFWwindow* window, const glm::vec3& sphereCenter, float sphereRadius) {
    // 屏幕坐标转NDC
    float x = (2.0f * (float)mouseX) / SCR_WIDTH - 1.0f;
    float y = 1.0f - (2.0f * (float)mouseY) / SCR_HEIGHT;
    glm::vec3 ray_nds = glm::vec3(x, y, 1.0f);

    // NDC转世界空间射线
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -35.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 200.0f);
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0f, 1.0f);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
    glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(view) * ray_eye));
    glm::vec3 ray_origin = glm::vec3(0.0f, 0.0f, 35.0f);

    // 精准相交检测（调用CPU端的raySphereIntersect）
    float t0, t1;
    return raySphereIntersect(ray_origin, ray_dir, sphereCenter, sphereRadius, t0, t1) && t0 > 0.0f;
}

// 鼠标点击回调（精准选中）
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (preciseRaySphereHit(xpos, ypos, window, sunPos, SUN_RADIUS)) {
            std::cout << "selected sun" << std::endl;
        } else if (preciseRaySphereHit(xpos, ypos, window, earthPos, EARTH_RADIUS)) {
            std::cout << "selected earth" << std::endl;
        } else if (preciseRaySphereHit(xpos, ypos, window, moonPos, MOON_RADIUS)) {
            std::cout << "selected moon" << std::endl;
        } else {
            std::cout << "no selected" << std::endl;
        }
    }
}

// 绘制天体（传递所有精准参数）
void drawPreciseCelestialBody(unsigned int texture, int objectType, const glm::vec3& pos, float scale) {
    glUseProgram(shaderProgram);

    // 模型矩阵（精准变换）
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, glm::vec3(scale));

    // 视图/投影矩阵（固定摄像机，精准视角）
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -35.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 200.0f);

    // 传递所有精准参数到着色器
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderProgram, "objectType"), objectType);
    glUniform3fv(glGetUniformLocation(shaderProgram, "sunPos"), 1, glm::value_ptr(sunPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "earthPos"), 1, glm::value_ptr(earthPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "moonPos"), 1, glm::value_ptr(moonPos));
    glUniform1f(glGetUniformLocation(shaderProgram, "sunRadius"), SUN_RADIUS);
    glUniform1f(glGetUniformLocation(shaderProgram, "earthRadius"), EARTH_RADIUS);
    glUniform1f(glGetUniformLocation(shaderProgram, "moonRadius"), MOON_RADIUS);
    glUniform1f(glGetUniformLocation(shaderProgram, "shadowBias"), SHADOW_BIAS);

    // 绑定纹理（精准采样）
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // 绘制高精度球体
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int main() {
    // 初始化GLFW（强制高精度上下文）
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x抗锯齿，提升精准度

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Precise Celestial Shadow", NULL, NULL);
    if (!window) {
        std::cerr << "GLFW窗口创建失败" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD初始化失败" << std::endl;
        return -1;
    }

    // 初始化高精度资源
    generateHighPrecisionSphere(1.0f, 64, 64); // 64*64高精度球体
    compileShaders();
    sunTexture = loadTexture("sun.bmp");
    earthTexture = loadTexture("earth.bmp");
    moonTexture = loadTexture("moon.bmp");

    // 启用深度测试（精准深度）
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE); // 启用抗锯齿
    glEnable(GL_CULL_FACE);   // 启用背面剔除（提升性能）
    glCullFace(GL_BACK);

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // 清空缓冲区（精准颜色）
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 更新时间和天体位置（精准浮点运算）
        float timeValue = (float)glfwGetTime();
        
        // 太阳固定在中心
        sunPos = glm::vec3(0.0f, 0.0f, 0.0f);
        
        // 地球绕太阳公转（精准角度计算）
        float earthAngle = timeValue * 0.3f; // 放慢速度，便于观察精准阴影
        earthPos = glm::vec3(
            cos(earthAngle) * EARTH_ORBIT_RADIUS,
            0.0f,
            sin(earthAngle) * EARTH_ORBIT_RADIUS
        );
        
        // 月亮绕地球公转（精准角度计算）
        float moonAngle = timeValue * 1.2f;
        moonPos = earthPos + glm::vec3(
            cos(moonAngle) * MOON_ORBIT_RADIUS,
            0.0f,
            sin(moonAngle) * MOON_ORBIT_RADIUS
        );

        // 绘制精准天体
        drawPreciseCelestialBody(sunTexture, 0, sunPos, SUN_RADIUS);    // 太阳
        drawPreciseCelestialBody(earthTexture, 1, earthPos, EARTH_RADIUS); // 地球
        drawPreciseCelestialBody(moonTexture, 2, moonPos, MOON_RADIUS);  // 月亮

        // 交换缓冲区（双缓冲保证精准显示）
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 释放资源
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &sunTexture);
    glDeleteTextures(1, &earthTexture);
    glDeleteTextures(1, &moonTexture);

    glfwTerminate();
    return 0;
}