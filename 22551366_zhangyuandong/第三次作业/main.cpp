#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#include <iostream>
#include <vector>
#include <string>

// 全局变量
GLFWwindow *window;
GLFWmonitor *primaryMonitor;
const GLFWvidmode *videoMode;
unsigned int shaderProgram;
unsigned int uiShaderProgram; // 2D UI着色器
unsigned int VAO, VBO, EBO;
unsigned int uiVAO, uiVBO; // 2D UI顶点缓冲
std::vector<glm::vec3> vertices;
std::vector<glm::vec2> texCoords;
std::vector<glm::vec3> normals;
std::vector<unsigned int> indices;
unsigned int textureID = 0;

// 视角控制
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;
float lastX = 400.0f, lastY = 300.0f;
float fov = 45.0f;
float deltaTime = 0.0f, lastFrame = 0.0f;

// 操作提示文本
const std::vector<std::string> operationTips = {
    "【操作提示】",
    "W/S/A/D：前后左右移动视角",
    "鼠标拖动：旋转视角",
    "滚轮：缩放视角",
    "ESC：退出程序",
    "当前光源：1个方向光 + 3个点光源"};

// 3D模型着色器（包含4个光源：1方向光 + 3点光源）
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    layout (location = 2) in vec3 aNormal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec2 TexCoord;
    out vec3 Normal;
    out vec3 FragPos;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoord = aTexCoord;
    }
)";

const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;
    in vec3 Normal;
    in vec3 FragPos;

    uniform sampler2D texture1;
    uniform vec3 viewPos;

    // 方向光（1个）
    struct DirLight {
        vec3 direction;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
    };
    uniform DirLight dirLight;

    // 点光源（3个，组成数组）
    struct PointLight {
        vec3 position;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
        float constant;
        float linear;
        float quadratic;
    };
    #define POINT_LIGHT_NUM 3
    uniform PointLight pointLights[POINT_LIGHT_NUM];

    // 计算方向光贡献
    vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
        vec3 lightDir = normalize(-light.direction);
        // 漫反射
        float diff = max(dot(normal, lightDir), 0.0);
        // 镜面反射
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        // 合并分量
        vec3 ambient = light.ambient * vec3(texture(texture1, TexCoord));
        vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoord));
        vec3 specular = light.specular * spec * vec3(1.0); // 镜面高光用白色
        return (ambient + diffuse + specular);
    }

    // 计算单个点光源贡献
    vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
        vec3 lightDir = normalize(light.position - fragPos);
        // 漫反射
        float diff = max(dot(normal, lightDir), 0.0);
        // 镜面反射
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        // 衰减计算
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
        // 合并分量
        vec3 ambient = light.ambient * vec3(texture(texture1, TexCoord));
        vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoord));
        vec3 specular = light.specular * spec * vec3(1.0);
        // 应用衰减
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        return (ambient + diffuse + specular);
    }

    void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);

        // 1. 方向光贡献
        vec3 result = CalcDirLight(dirLight, norm, viewDir);

        // 2. 3个点光源贡献
        for(int i = 0; i < POINT_LIGHT_NUM; i++) {
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
        }

        FragColor = vec4(result, 1.0);
    }
)";

// 2D UI着色器（用于绘制提示文本背景）
const char *uiVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 projection;

    out vec2 TexCoord;

    void main() {
        gl_Position = projection * vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char *uiFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;

    uniform vec3 textColor;
    uniform float alpha;

    void main() {
        FragColor = vec4(textColor, alpha);
    }
)";

// 错误处理与回调
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = normalize(front);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}
bool isModelRotating = true;
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        isModelRotating = !isModelRotating; // 切换旋转状态
        glfwWaitEvents();                   // 避免重复触发
    }
}

// 加载OBJ模型
bool loadOBJ(const std::string &path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
    {
        std::cerr << warn << err << std::endl;
        return false;
    }

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            // 顶点位置
            vertices.push_back(glm::vec3(
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]));
            // 纹理坐标
            if (attrib.texcoords.size() > 0)
            {
                texCoords.push_back(glm::vec2(
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]));
            }
            else
            {
                texCoords.push_back(glm::vec2(0.0f, 0.0f));
            }
            // 法线
            if (attrib.normals.size() > 0)
            {
                normals.push_back(glm::vec3(
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]));
            }
            else
            {
                normals.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
            }
            indices.push_back(indices.size());
        }
    }
    return true;
}

// 加载纹理
unsigned int loadTexture(const std::string &path)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // 纹理参数设置
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 加载图片
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        // 若纹理加载失败，创建默认白色纹理
        unsigned char defaultData[] = {255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, defaultData);
    }
    stbi_image_free(data);
    return texture;
}

// 编译着色器工具函数
unsigned int compileShaderProgram(const char *vertSource, const char *fragSource)
{
    // 编译顶点着色器
    unsigned int vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSource, NULL);
    glCompileShader(vertShader);
    // 检查顶点着色器编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Compilation Failed:\n"
                  << infoLog << std::endl;
    }

    // 编译片段着色器
    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSource, NULL);
    glCompileShader(fragShader);
    // 检查片段着色器编译错误
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Compilation Failed:\n"
                  << infoLog << std::endl;
    }

    // 链接着色器程序
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    // 检查链接错误
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader Program Linking Failed:\n"
                  << infoLog << std::endl;
    }

    // 删除临时着色器对象
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}

// 初始化2D UI
void initUI()
{
    // 编译UI着色器
    uiShaderProgram = compileShaderProgram(uiVertexShaderSource, uiFragmentShaderSource);

    // 创建UI顶点缓冲
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);

    // 6个顶点组成2个三角形（替代GL_QUADS，核心模式兼容）
    float uiVertices[] = {
        // 位置          // 纹理坐标
        0.0f, 0.0f, 0.0f, 0.0f,     // 左下
        300.0f, 0.0f, 1.0f, 0.0f,   // 右下
        300.0f, 200.0f, 1.0f, 1.0f, // 右上
        0.0f, 0.0f, 0.0f, 0.0f,     // 左下（复用）
        300.0f, 200.0f, 1.0f, 1.0f, // 右上（复用）
        0.0f, 200.0f, 0.0f, 1.0f    // 左上
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), uiVertices, GL_STATIC_DRAW);

    // 配置顶点属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

// 初始化（全屏+4光源配置）
void init()
{
    // GLFW初始化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 获取主显示器和视频模式（实现全屏）
    primaryMonitor = glfwGetPrimaryMonitor();
    videoMode = glfwGetVideoMode(primaryMonitor);
    glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);

    // 创建全屏窗口
    window = glfwCreateWindow(videoMode->width, videoMode->height, "4-Light OBJ Model Viewer", primaryMonitor, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create fullscreen GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD加载
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    // 编译3D模型着色器
    shaderProgram = compileShaderProgram(vertexShaderSource, fragmentShaderSource);

    // 加载模型（替换为你的OBJ路径）
    if (!loadOBJ("model.obj"))
    {
        std::cerr << "Failed to load OBJ model (请替换为有效OBJ路径)" << std::endl;
        // 模型加载失败仍继续运行（显示默认顶点）
    }

    // 加载纹理（替换为你的纹理路径，无纹理则使用默认白色纹理）
    textureID = loadTexture("texture.png");

    // 绑定3D模型VAO/VBO/EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    // 合并顶点数据（位置+纹理+法线）
    std::vector<float> vertexData;
    for (size_t i = 0; i < vertices.size(); i++)
    {
        vertexData.push_back(vertices[i].x);
        vertexData.push_back(vertices[i].y);
        vertexData.push_back(vertices[i].z);
        vertexData.push_back(texCoords[i].x);
        vertexData.push_back(texCoords[i].y);
        vertexData.push_back(normals[i].x);
        vertexData.push_back(normals[i].y);
        vertexData.push_back(normals[i].z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // 配置顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // 初始化2D UI和深度测试
    initUI();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); // 启用混合，实现半透明UI
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// 渲染循环（设置4个光源并绘制）
void render()
{
    while (!glfwWindowShouldClose(window))
    {
        // 计算帧时间差
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput(window);

        // 清空缓冲
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 绘制3D模型
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 模型/视图/投影矩阵
        glm::mat4 model = glm::mat4(1.0f);
        if (isModelRotating)
        {
            model = glm::rotate(model, (float)glfwGetTime() * glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)videoMode->width / videoMode->height, 0.1f, 100.0f);

        // 设置矩阵uniform
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

        // 1. 设置方向光（第1个光源）
        glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.direction"), 1, glm::value_ptr(glm::vec3(-0.5f, -1.0f, -0.3f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.ambient"), 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.2f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.diffuse"), 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, "dirLight.specular"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));

        // 2. 设置3个点光源（第2-4个光源）
        // 点光源1：右侧
        std::string pointLight1 = "pointLights[0]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight1 + ".position").c_str()), 1, glm::value_ptr(glm::vec3(5.0f, 0.0f, 0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight1 + ".ambient").c_str()), 1, glm::value_ptr(glm::vec3(0.2f, 0.0f, 0.0f))); // 红光
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight1 + ".diffuse").c_str()), 1, glm::value_ptr(glm::vec3(0.8f, 0.0f, 0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight1 + ".specular").c_str()), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight1 + ".constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight1 + ".linear").c_str()), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight1 + ".quadratic").c_str()), 0.032f);

        // 点光源2：左侧
        std::string pointLight2 = "pointLights[1]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight2 + ".position").c_str()), 1, glm::value_ptr(glm::vec3(-5.0f, 0.0f, 0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight2 + ".ambient").c_str()), 1, glm::value_ptr(glm::vec3(0.0f, 0.2f, 0.0f))); // 绿光
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight2 + ".diffuse").c_str()), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight2 + ".specular").c_str()), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight2 + ".constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight2 + ".linear").c_str()), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight2 + ".quadratic").c_str()), 0.032f);

        // 点光源3：上方
        std::string pointLight3 = "pointLights[2]";
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight3 + ".position").c_str()), 1, glm::value_ptr(glm::vec3(0.0f, 5.0f, 0.0f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight3 + ".ambient").c_str()), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.2f))); // 蓝光
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight3 + ".diffuse").c_str()), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.8f)));
        glUniform3fv(glGetUniformLocation(shaderProgram, (pointLight3 + ".specular").c_str()), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight3 + ".constant").c_str()), 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight3 + ".linear").c_str()), 0.09f);
        glUniform1f(glGetUniformLocation(shaderProgram, (pointLight3 + ".quadratic").c_str()), 0.032f);

        // 绘制模型
        if (!indices.empty())
        {
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        // 交换缓冲并轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

// 清理资源
void cleanup()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &uiVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &uiVBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(uiShaderProgram);
    glfwTerminate();
}

// 主函数
int main()
{
    init();
    render();
    cleanup();
    return 0;
}