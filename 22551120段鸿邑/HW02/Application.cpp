#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include<vector>
// stb_image 配置
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define M_PI 3.14159265358979323846
// 全局变量
GLFWwindow* window = nullptr;
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// 着色器程序ID
unsigned int sunShaderProgram, earthShaderProgram;
// 纹理ID
unsigned int sunTex, earthDiffuseTex, earthNormalTex;
// 球体VAO/VBO/EBO
unsigned int sphereVAO, sphereVBO, sphereEBO;
unsigned int sphereIndexCount = 0;

// 相机参数
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 15.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// 视图矩阵+投影矩阵（全局可访问，供鼠标回调使用）
glm::mat4 view;
glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);

// 球体信息结构体：存储名称、世界空间球心、实际半径
struct SphereInfo {
    std::string name;       // 球体名称（太阳/地球）
    glm::vec3 worldCenter;  // 世界空间球心
    float worldRadius;      // 世界空间实际半径
};
std::vector<SphereInfo> sphereList; // 球体列表，存储太阳和地球的信息

// 射线结构体：起点 + 归一化方向
struct Ray {
    glm::vec3 origin;   // 射线起点（相机位置）
    glm::vec3 dir;      // 射线方向（归一化后）
};

// 射线与球体相交检测
// ray：世界空间射线
// sphereCenter：球体世界空间球心
// sphereRadius：球体世界空间半径
bool raySphereIntersect(const Ray& ray, const glm::vec3& sphereCenter, float sphereRadius) {
    // 计算射线起点到球心的向量
    glm::vec3 oc = ray.origin - sphereCenter;
    // 二次方程系数：t² + 2*b*t + c = 0
    float a = glm::dot(ray.dir, ray.dir); // 因dir已归一化，a=1.0f，可简化
    float b = 2.0f * glm::dot(oc, ray.dir);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    // 判别式
    float discriminant = b * b - 4 * a * c;

    // 判别式<0：无交点；≥0：有1个或2个交点（相交）
    if (discriminant < 0) {
        return false;
    }
    else {
        // 计算最近的交点距离t
        float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
        // t>0：交点在射线前方（有效交点）；t<0：交点在射线后方（无效）
        return t > 0.0001f; // 加微小偏移，避免相机在球体内的误判
    }
}

// 窗口坐标转世界空间射线
// mouseX/mouseY：GLFW获取的鼠标窗口坐标
// viewMat：视图矩阵
// projMat：投影矩阵
// windowWidth/windowHeight：窗口尺寸
Ray screenToWorldRay(float mouseX, float mouseY, const glm::mat4& viewMat, const glm::mat4& projMat, unsigned int windowWidth, unsigned int windowHeight) {
    Ray ray;
    // 步骤1：窗口坐标（像素）转标准化设备坐标（NDC，范围[-1,1]）
    // GLFW窗口坐标：左上角为(0,0)，Y轴向下；NDC：中心为(0,0)，Y轴向上
    float x = (2.0f * mouseX) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / windowHeight; // 翻转Y轴
    float z = -1.0f; // 近裁剪面Z值
    glm::vec4 ndcNear = glm::vec4(x, y, z, 1.0f);
    z = 1.0f; // 远裁剪面Z值
    glm::vec4 ndcFar = glm::vec4(x, y, z, 1.0f);

    // 步骤2：NDC转世界空间（乘以投影矩阵逆 + 视图矩阵逆）
    glm::mat4 invProjView = glm::inverse(projMat * viewMat);
    // 近裁剪面世界空间坐标
    glm::vec4 worldNear = invProjView * ndcNear;
    // 远裁剪面世界空间坐标
    glm::vec4 worldFar = invProjView * ndcFar;

    // 步骤3：透视除法（齐次坐标转笛卡尔坐标，除以w分量）
    worldNear /= worldNear.w;
    worldFar /= worldFar.w;

    // 步骤4：构建射线（起点=相机位置，方向=远-近，归一化）
    ray.origin = cameraPos;
    ray.dir = glm::normalize(glm::vec3(worldFar - worldNear));

    return ray;
}

// GLFW鼠标按钮回调函数
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // 仅处理左键按下事件（GLFW_PRESS：按下；GLFW_RELEASE：释放）
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // 步骤1：获取鼠标当前窗口坐标
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // 步骤2：获取当前的视图矩阵和投影矩阵（全局变量已更新）
        glm::mat4 currentView = view;
        glm::mat4 currentProj = projection;

        // 步骤3：转换为世界空间射线
        Ray ray = screenToWorldRay((float)mouseX, (float)mouseY, currentView, currentProj, SCR_WIDTH, SCR_HEIGHT);

        // 步骤4：遍历所有球体，检测相交（记录最近的球体）
        std::string hitSphereName = "";
        float minT = FLT_MAX; // 最近交点的t值
        for (const auto& sphere : sphereList) {
            // 先判断是否相交
            glm::vec3 oc = ray.origin - sphere.worldCenter;
            float a = glm::dot(ray.dir, ray.dir);
            float b = 2.0f * glm::dot(oc, ray.dir);
            float c = glm::dot(oc, oc) - sphere.worldRadius * sphere.worldRadius;
            float discriminant = b * b - 4 * a * c;

            if (discriminant >= 0) {
                float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
                if (t > 0.0001f && t < minT) {
                    minT = t;
                    hitSphereName = sphere.name;
                }
            }
        }

        // 步骤5：若命中球体，打印名称
        if (!hitSphereName.empty()) {
            std::cout << "点击了球体：" << hitSphereName << std::endl;
        }
        else {
            std::cout << "未点击到任何球体" << std::endl;
        }
    }
}

// -------------------------- 辅助函数声明 --------------------------
// GLFW错误回调
void glfwErrorCallback(int error, const char* description);
// 窗口大小调整回调
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
// 编译着色器
unsigned int compileShader(unsigned int type, const char* source);
// 创建着色器程序
unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
// 生成球体数据（顶点、法线、纹理坐标、索引）
void generateSphere(float radius, unsigned int sectors, unsigned int stacks);
// 加载纹理
unsigned int loadTexture(const char* path, bool isNormalMap = false);
// 初始化所有资源
bool initResources();
// 渲染帧
void renderFrame();
// 释放资源
void releaseResources();

// -------------------------- 着色器源码 --------------------------
// 太阳着色器（自发光，仅显示贴图，无需光照）
const char* sunVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 2) in vec2 aTexCoords;

    out vec2 TexCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        TexCoords = aTexCoords;
        gl_Position = projection * view * model * vec4(aPos, 1.0f);
    }
)";

const char* sunFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D sunTexture;

    void main()
    {
        vec3 sunColor = texture(sunTexture, TexCoords).rgb;
        FragColor = vec4(sunColor * 1.2f, 1.0f); // 自发光增强
    }
)";

// 地球着色器（支持漫反射贴图+法线贴图+Phong光照）
const char* earthVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    layout (location = 3) in vec3 aTangent;
    layout (location = 4) in vec3 aBitangent;

    out VS_OUT {
        vec2 TexCoords;
        vec3 FragPos;
        mat3 TBN;
    } vs_out;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        vs_out.FragPos = vec3(model * vec4(aPos, 1.0f));
        vs_out.TexCoords = aTexCoords;

        // 计算TBN矩阵（切线空间 -> 世界空间）
        vec3 T = normalize(vec3(model * vec4(aTangent, 0.0f)));
        vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0f)));
        vec3 N = normalize(vec3(model * vec4(aNormal, 0.0f)));
        // 修正TBN矩阵正交性
        T = normalize(T - dot(T, N) * N);
        B = cross(N, T);
        vs_out.TBN = mat3(T, B, N);

        gl_Position = projection * view * model * vec4(aPos, 1.0f);
    }
)";

const char* earthFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in VS_OUT {
        vec2 TexCoords;
        vec3 FragPos;
        mat3 TBN;
    } fs_in;

    // 纹理
    uniform sampler2D earthDiffuse;
    uniform sampler2D earthNormal;

    // 光照参数
    uniform vec3 lightPos;     // 太阳位置（光源位置）
    uniform vec3 viewPos;      // 相机位置
    uniform vec3 lightColor;   // 光源颜色
    uniform float lightIntensity; // 光源强度

    void main()
    {
        // 1. 采样法线贴图并转换为世界空间法线
        vec3 normalMap = texture(earthNormal, fs_in.TexCoords).rgb;
        normalMap = normalize(normalMap * 2.0f - 1.0f); // 从[0,1]转换到[-1,1]
        vec3 normal = normalize(fs_in.TBN * normalMap); // 切线空间 -> 世界空间

        // 2. 采样漫反射贴图
        vec3 diffuseColor = texture(earthDiffuse, fs_in.TexCoords).rgb;

        // 3. Phong光照计算
        // 环境光
        vec3 ambient = 0.1f * diffuseColor;

        // 漫反射
        vec3 lightDir = normalize(lightPos - fs_in.FragPos);
        float diff = max(dot(normal, lightDir), 0.0f);
        vec3 diffuse = diff * diffuseColor * lightColor * lightIntensity;

        // 镜面反射
        vec3 viewDir = normalize(viewPos - fs_in.FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
        vec3 specular = 0.2f * spec * lightColor; // 镜面强度

        // 最终颜色
        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0f);
    }
)";

// -------------------------- 辅助函数实现 --------------------------
void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // 更新投影矩阵
    projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);
}

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    // 检查编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error (" << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") << "):\n" << infoLog << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertexShader == 0 || fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // 检查链接错误
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader Program Link Error:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    // 删除中间着色器
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

void generateSphere(float radius, unsigned int sectors, unsigned int stacks)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    float sectorAngle, stackAngle;

    // 生成顶点（位置、法线、纹理坐标、切线、副切线）
    for (unsigned int i = 0; i <= stacks; ++i)
    {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (unsigned int j = 0; j <= sectors; ++j)
        {
            sectorAngle = j * sectorStep;

            // 位置
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // 法线
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);

            // 纹理坐标
            s = (float)j / sectors;
            t = (float)i / stacks;
            vertices.push_back(s);
            vertices.push_back(t);

            // 切线
            float tx = -sinf(sectorAngle);
            float ty = cosf(sectorAngle);
            float tz = 0.0f;
            vertices.push_back(tx);
            vertices.push_back(ty);
            vertices.push_back(tz);

            // 副切线（法线叉乘切线）
            float bx = ny * tz - nz * ty;
            float by = nz * tx - nx * tz;
            float bz = nx * ty - ny * tx;
            vertices.push_back(bx);
            vertices.push_back(by);
            vertices.push_back(bz);
        }
    }

    // 生成索引
    unsigned int k1, k2;
    for (unsigned int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            // 两个三角形
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != stacks - 1)
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    sphereIndexCount = indices.size();

    // 绑定VAO/VBO/EBO
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 顶点位置（location 0）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法线（location 1）
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标（location 2）
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // 切线（location 3）
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    // 副切线（location 4）
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);
}

unsigned int loadTexture(const char* path, bool isNormalMap)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data == nullptr)
    {
        std::cerr << "Texture load failed: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    GLenum format;
    if (nrComponents == 1)
        format = GL_RED;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 4)
        format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // 纹理环绕和过滤设置
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    if (isNormalMap)
    {
        // 法线贴图使用线性过滤
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    stbi_image_free(data);
    return textureID;
}

bool initResources()
{
    // 1. 生成球体（半径1，36扇区，18堆叠，足够平滑）
    generateSphere(1.0f, 36, 18);

    // 2. 创建着色器程序
    sunShaderProgram = createShaderProgram(sunVertexShaderSource, sunFragmentShaderSource);
    earthShaderProgram = createShaderProgram(earthVertexShaderSource, earthFragmentShaderSource);
    if (sunShaderProgram == 0 || earthShaderProgram == 0)
    {
        return false;
    }

    // 3. 加载纹理
    stbi_set_flip_vertically_on_load(true); // 翻转纹理（OpenGL纹理坐标Y轴向下）
    sunTex = loadTexture("E:/OpenGLLearning/OpenGLHW02/Resources/太阳_2K.jpg"); // 太阳漫反射贴图
    earthDiffuseTex = loadTexture("E:/OpenGLLearning/OpenGLHW02/Resources/世界地球日地图_2K.jpg"); // 地球漫反射贴图
    earthNormalTex = loadTexture("E:/OpenGLLearning/OpenGLHW02/Resources/地球法线贴图_2K.jpg", true); // 地球法线贴图
    if (sunTex == 0 || earthDiffuseTex == 0 || earthNormalTex == 0)
    {
        return false;
    }

    // 4. 开启深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // 5. 设置清屏颜色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 黑色背景（模拟宇宙）

    return true;
}

void renderFrame()
{
    // 1. 清屏（颜色缓冲+深度缓冲）
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. 计算视图矩阵（更新全局view矩阵）
    view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    // 3. 获取时间（用于地球公转）
    float time = (float)glfwGetTime();

    // -------------------------- 渲染太阳 --------------------------
    glUseProgram(sunShaderProgram);
    // 太阳模型矩阵：缩放（比地球大），无位移（太阳系中心）
    glm::mat4 sunModel = glm::mat4(1.0f);
    sunModel = glm::scale(sunModel, glm::vec3(2.0f, 2.0f, 2.0f)); // 太阳半径放大2倍

    // 设置太阳着色器uniform
    unsigned int sunModelLoc = glGetUniformLocation(sunShaderProgram, "model");
    unsigned int sunViewLoc = glGetUniformLocation(sunShaderProgram, "view");
    unsigned int sunProjLoc = glGetUniformLocation(sunShaderProgram, "projection");
    glUniformMatrix4fv(sunModelLoc, 1, GL_FALSE, glm::value_ptr(sunModel));
    glUniformMatrix4fv(sunViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(sunProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // 绑定太阳纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sunTex);
    glUniform1i(glGetUniformLocation(sunShaderProgram, "sunTexture"), 0);

    // 绘制太阳
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // -------------------------- 渲染地球 --------------------------
    glUseProgram(earthShaderProgram);
    // 地球模型矩阵：公转+缩放（比太阳小）
    glm::mat4 earthModel = glm::mat4(1.0f);
    // 公转：绕Y轴旋转，轨道半径8.0f
    glm::vec3 earthWorldPos = glm::vec3(cos(time) * 8.0f, 0.0f, sin(time) * 8.0f);
    earthModel = glm::translate(earthModel, earthWorldPos);
    earthModel = glm::scale(earthModel, glm::vec3(0.5f, 0.5f, 0.5f)); // 地球半径缩小为0.5倍

    // 设置地球着色器uniform（模型/视图/投影矩阵）
    unsigned int earthModelLoc = glGetUniformLocation(earthShaderProgram, "model");
    unsigned int earthViewLoc = glGetUniformLocation(earthShaderProgram, "view");
    unsigned int earthProjLoc = glGetUniformLocation(earthShaderProgram, "projection");
    glUniformMatrix4fv(earthModelLoc, 1, GL_FALSE, glm::value_ptr(earthModel));
    glUniformMatrix4fv(earthViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(earthProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // 设置光照参数（太阳作为点光源）
    glm::vec3 sunPos = glm::vec3(0.0f, 0.0f, 0.0f); // 太阳在世界原点
    glUniform3fv(glGetUniformLocation(earthShaderProgram, "lightPos"), 1, glm::value_ptr(sunPos));
    glUniform3fv(glGetUniformLocation(earthShaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
    glUniform3f(glGetUniformLocation(earthShaderProgram, "lightColor"), 1.0f, 0.9f, 0.7f); // 暖黄色太阳光
    glUniform1f(glGetUniformLocation(earthShaderProgram, "lightIntensity"), 1.0f);

    // 绑定地球纹理
    // 漫反射纹理 -> GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthDiffuseTex);
    glUniform1i(glGetUniformLocation(earthShaderProgram, "earthDiffuse"), 0);
    // 法线纹理 -> GL_TEXTURE1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, earthNormalTex);
    glUniform1i(glGetUniformLocation(earthShaderProgram, "earthNormal"), 1);

    // 绘制地球
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // 解绑纹理和着色器
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    // 关键：实时更新地球的世界球心（因公转位置变化）
    if (!sphereList.empty() && sphereList.size() >= 2) {
        sphereList[1].worldCenter = earthWorldPos;
    }
}

void releaseResources()
{
    // 删除VAO/VBO/EBO
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    // 删除着色器程序
    glDeleteProgram(sunShaderProgram);
    glDeleteProgram(earthShaderProgram);

    // 删除纹理
    glDeleteTextures(1, &sunTex);
    glDeleteTextures(1, &earthDiffuseTex);
    glDeleteTextures(1, &earthNormalTex);
}

// -------------------------- 主函数 --------------------------
int main()
{
    // 1. 初始化GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return -1;
    }

    // 2. 配置GLFW窗口
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 3. 创建窗口
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Simple Solar System (GLFW)", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "GLFW window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    // 设置鼠标按钮回调（关键：监听左键点击）
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // 4. 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "GLAD initialization failed!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 5. 初始化资源
    if (!initResources())
    {
        std::cerr << "Resource initialization failed!" << std::endl;
        releaseResources();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 6. 初始化球体列表（关键：填充太阳和地球信息）
    // 太阳信息
    SphereInfo sun;
    sun.name = "太阳";
    sun.worldCenter = glm::vec3(0.0f, 0.0f, 0.0f); // 太阳在世界原点
    sun.worldRadius = 1.0f * 2.0f; // 局部半径1.0f * 缩放2倍
    sphereList.push_back(sun);

    // 地球初始信息
    SphereInfo earth;
    earth.name = "地球";
    earth.worldCenter = glm::vec3(8.0f, 0.0f, 0.0f); // 初始公转位置
    earth.worldRadius = 1.0f * 0.5f; // 局部半径1.0f * 缩放0.5倍
    sphereList.push_back(earth);

    // 7. 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 渲染帧
        renderFrame();

        // 交换缓冲+处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 8. 释放资源
    releaseResources();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}