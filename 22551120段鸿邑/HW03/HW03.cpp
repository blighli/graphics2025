#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// ===================== 全局常量配置 =====================
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float MOUSE_SENSITIVITY = 0.1f;
const float SCROLL_SENSITIVITY = 0.1f;
const float MOVE_SPEED = 25.0f;

// ===================== 视点模式枚举 =====================
enum class ViewMode {
    MODEL_CENTERED,  // 以模型为中心（平移、旋转、缩放）
    VIEWPOINT_CENTERED  // 以视点为中心（场景漫游）
};

// ===================== 全局变量 =====================
// 视点模式
ViewMode currentViewMode = ViewMode::MODEL_CENTERED;
// 模型信息
glm::vec3 modelCenter = glm::vec3(0.0f);
float modelRadius = 5.0f;

// 模型中心模式参数
float mc_Yaw = -90.0f;
float mc_Pitch = 0.0f;
float mc_Distance = 10.0f;
glm::vec3 mc_ModelOffset = glm::vec3(0.0f);

// 视点中心模式参数
float vc_Yaw = -90.0f;
float vc_Pitch = 0.0f;
glm::vec3 vc_CameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
glm::vec3 vc_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 vc_CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// 鼠标状态
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// 时间参数
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ===================== 着色器类 =====================
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        // 1. 读取着色器文件
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // 允许异常抛出
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            vShaderFile.close();
            fShaderFile.close();
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. 编译顶点着色器
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vShaderCode, NULL);
        glCompileShader(vertexShader);
        checkCompileErrors(vertexShader, "VERTEX");

        // 3. 编译片段着色器
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
        glCompileShader(fragmentShader);
        checkCompileErrors(fragmentShader, "FRAGMENT");

        // 4. 链接着色器程序
        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 5. 删除临时着色器
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    // 激活着色器
    void use() {
        glUseProgram(ID);
    }

    // 统一变量设置函数
    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    // 检查着色器编译/链接错误
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[512];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILE_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 512, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINK_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

// ===================== 网格顶点结构 =====================
struct Vertex {
    glm::vec3 Position;  // 顶点位置
    glm::vec3 Normal;    // 顶点法线
    glm::vec2 TexCoords; // 纹理坐标
};

// ===================== 网格类 =====================
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
        this->vertices = vertices;
        this->indices = indices;
        setupMesh();
    }

    // 绘制网格
    void Draw(Shader& shader) {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    // 初始化网格缓冲区
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // 绑定VBO并写入顶点数据
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // 绑定EBO并写入索引数据
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 配置顶点位置属性
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // 配置顶点法线属性
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        // 配置顶点纹理坐标属性
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

// ===================== 模型类 =====================
class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;

    // 构造函数：加载模型
    Model(const char* path) {
        loadModel(path);
        calculateModelCenterAndRadius();
    }

    // 绘制模型
    void Draw(Shader& shader) {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shader);
        }
    }

    // 获取模型信息
    glm::vec3 getModelCenter() { return modelCenter; }
    float getModelRadius() { return modelRadius; }

private:
    // 加载模型（Assimp核心）
    void loadModel(std::string path) {
        Assimp::Importer importer;
        // Assimp后处理配置：三角化、生成法线、翻转UV、合并顶点
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices);

        // 检查加载是否成功
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        // 获取模型目录
        directory = path.substr(0, path.find_last_of('/'));
        // 递归处理所有节点
        processNode(scene->mRootNode, scene);
    }

    // 处理Assimp节点
    void processNode(aiNode* node, const aiScene* scene) {
        // 处理当前节点的所有网格
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // 递归处理子节点
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    // 转换Assimp网格为自定义网格
    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // 处理顶点数据
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            glm::vec3 vec;

            // 顶点位置
            vec.x = mesh->mVertices[i].x;
            vec.y = mesh->mVertices[i].y;
            vec.z = mesh->mVertices[i].z;
            vertex.Position = vec;

            // 顶点法线
            if (mesh->HasNormals()) {
                vec.x = mesh->mNormals[i].x;
                vec.y = mesh->mNormals[i].y;
                vec.z = mesh->mNormals[i].z;
                vertex.Normal = vec;
            }

            // 顶点纹理坐标
            if (mesh->mTextureCoords[0]) {
                glm::vec2 tex;
                tex.x = mesh->mTextureCoords[0][i].x;
                tex.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = tex;
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // 处理索引数据
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        return Mesh(vertices, indices);
    }

    // 计算模型中心和包围球半径
    void calculateModelCenterAndRadius() {
        if (meshes.empty() || meshes[0].vertices.empty()) {
            modelCenter = glm::vec3(0.0f);
            modelRadius = 5.0f;
            return;
        }

        // 计算轴对齐包围盒（AABB）
        glm::vec3 minPos = meshes[0].vertices[0].Position;
        glm::vec3 maxPos = meshes[0].vertices[0].Position;

        for (auto& mesh : meshes) {
            for (auto& vertex : mesh.vertices) {
                minPos = glm::min(minPos, vertex.Position);
                maxPos = glm::max(maxPos, vertex.Position);
            }
        }

        // 模型中心
        modelCenter = (minPos + maxPos) * 0.5f;
        // 包围球半径
        modelRadius = glm::length(maxPos - minPos) * 0.5f;
        // 初始化视点距离
        mc_Distance = modelRadius * 2.0f;
        vc_CameraPos = modelCenter + glm::vec3(0.0f, 0.0f, modelRadius * 2.0f);
    }
};

// ===================== 回调函数 =====================
// 窗口大小调整回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 鼠标移动回调
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= MOUSE_SENSITIVITY;
    yoffset *= MOUSE_SENSITIVITY;

    if (currentViewMode == ViewMode::MODEL_CENTERED) {
        // 模型中心模式：旋转视点
        mc_Yaw += xoffset;
        mc_Pitch += yoffset;

        // 限制俯仰角
        if (mc_Pitch > 89.0f) mc_Pitch = 89.0f;
        if (mc_Pitch < -89.0f) mc_Pitch = -89.0f;
    }
    else if (currentViewMode == ViewMode::VIEWPOINT_CENTERED) {
        // 视点中心模式：旋转视线
        vc_Yaw += xoffset;
        vc_Pitch += yoffset;

        // 限制俯仰角
        if (vc_Pitch > 89.0f) vc_Pitch = 89.0f;
        if (vc_Pitch < -89.0f) vc_Pitch = -89.0f;

        // 更新视线方向
        glm::vec3 front;
        front.x = cos(glm::radians(vc_Yaw)) * cos(glm::radians(vc_Pitch));
        front.y = sin(glm::radians(vc_Pitch));
        front.z = sin(glm::radians(vc_Yaw)) * cos(glm::radians(vc_Pitch));
        vc_CameraFront = glm::normalize(front);
    }
}

// 鼠标滚轮回调
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentViewMode == ViewMode::MODEL_CENTERED) {
        // 模型中心模式：缩放（调整视点距离）
        mc_Distance -= (float)yoffset * SCROLL_SENSITIVITY * mc_Distance;
        if (mc_Distance < modelRadius * 0.1f) mc_Distance = modelRadius * 0.1f;
    }
}

// 输入处理函数
void processInput(GLFWwindow* window) {
    // 退出程序（ESC键）
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // 切换视点模式（C键，单次触发，不重复）
    static bool cKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cKeyPressed) {
            currentViewMode = (currentViewMode == ViewMode::MODEL_CENTERED) ? ViewMode::VIEWPOINT_CENTERED : ViewMode::MODEL_CENTERED;
            std::cout << "当前模式：" << (currentViewMode == ViewMode::MODEL_CENTERED ? "模型中心模式" : "视点中心漫游模式") << std::endl;
            cKeyPressed = true;
        }
    }
    else {
        cKeyPressed = false;
    }

    float speed = MOVE_SPEED * deltaTime;

    if (currentViewMode == ViewMode::MODEL_CENTERED) {
        // 模型中心模式：平移模型（W/S/A/D）
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            mc_ModelOffset.y += speed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            mc_ModelOffset.y -= speed;
        }

        // 计算右方向向量
        glm::vec3 mc_Front = glm::normalize(glm::vec3(
            cos(glm::radians(mc_Yaw)) * cos(glm::radians(mc_Pitch)),
            sin(glm::radians(mc_Pitch)),
            sin(glm::radians(mc_Yaw)) * cos(glm::radians(mc_Pitch))
        ));
        glm::vec3 mc_Right = glm::normalize(glm::cross(mc_Front, glm::vec3(0.0f, 1.0f, 0.0f)));

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            mc_ModelOffset -= mc_Right * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            mc_ModelOffset += mc_Right * speed;
        }
    }
    else if (currentViewMode == ViewMode::VIEWPOINT_CENTERED) {
        // 视点中心模式：第一人称漫游（W/S/A/D/空格/左Shift）
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            vc_CameraPos += speed * vc_CameraFront;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            vc_CameraPos -= speed * vc_CameraFront;
        }
        glm::vec3 vc_Right = glm::normalize(glm::cross(vc_CameraFront, vc_CameraUp));
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            vc_CameraPos -= vc_Right * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            vc_CameraPos += vc_Right * speed;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            vc_CameraPos += speed * vc_CameraUp;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            vc_CameraPos -= speed * vc_CameraUp;
        }
    }
}

// ===================== 主函数 =====================
int main() {
    // 1. 初始化GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OBJ Multi-Light Viewer (C键切换模式)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 隐藏鼠标并捕获
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 3. 加载GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 4. 启用深度测试
    glEnable(GL_DEPTH_TEST);

    // 5. 加载着色器（注意：确保lighting.vs和lighting.fs在项目目录下）
    Shader lightingShader("E:/OpenGLLearning/OpenGLHW02/src/lighting.vs", "E:/OpenGLLearning/OpenGLHW02/src/lighting.fs");
    if (lightingShader.ID == 0) {
        std::cout << "Failed to load shader" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 6. 加载OBJ模型
    Model* model = nullptr;
    try {
        model = new Model("E:/OpenGLLearning/OpenGLHW02/Resources/teapot.obj"); 
        std::cout << "模型加载成功！中心：(" << modelCenter.x << "," << modelCenter.y << "," << modelCenter.z << ")" << std::endl;
        std::cout << "模型半径：" << modelRadius << std::endl;
    }
    catch (std::exception& e) {
        std::cout << "模型加载失败：" << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    // 7. 配置多光源参数（所有setVec3均传入glm::vec3对象，严格按你的要求修正）
    lightingShader.use();
    // 材质参数
    lightingShader.setVec3("material.ambient", glm::vec3(0.3f, 0.3f, 0.3f));
    lightingShader.setVec3("material.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    lightingShader.setVec3("material.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("material.shininess", 32.0f);

    // 平行光
    lightingShader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
    lightingShader.setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("dirLight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    lightingShader.setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

    // 4个点光源（围绕模型分布）
    lightingShader.setInt("pointLightCount", 4);
    // 点光源1
    lightingShader.setVec3("pointLights[0].position", modelCenter + glm::vec3(5.0f, 0.0f, 0.0f));
    lightingShader.setVec3("pointLights[0].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("pointLights[0].diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    lightingShader.setVec3("pointLights[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("pointLights[0].constant", 1.0f);
    lightingShader.setFloat("pointLights[0].linear", 0.09f);
    lightingShader.setFloat("pointLights[0].quadratic", 0.032f);
    // 点光源2
    lightingShader.setVec3("pointLights[1].position", modelCenter + glm::vec3(-5.0f, 0.0f, 0.0f));
    lightingShader.setVec3("pointLights[1].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("pointLights[1].diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    lightingShader.setVec3("pointLights[1].specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("pointLights[1].constant", 1.0f);
    lightingShader.setFloat("pointLights[1].linear", 0.09f);
    lightingShader.setFloat("pointLights[1].quadratic", 0.032f);
    // 点光源3
    lightingShader.setVec3("pointLights[2].position", modelCenter + glm::vec3(0.0f, 5.0f, 0.0f));
    lightingShader.setVec3("pointLights[2].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("pointLights[2].diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    lightingShader.setVec3("pointLights[2].specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("pointLights[2].constant", 1.0f);
    lightingShader.setFloat("pointLights[2].linear", 0.09f);
    lightingShader.setFloat("pointLights[2].quadratic", 0.032f);
    // 点光源4
    lightingShader.setVec3("pointLights[3].position", modelCenter + glm::vec3(0.0f, 0.0f, 5.0f));
    lightingShader.setVec3("pointLights[3].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    lightingShader.setVec3("pointLights[3].diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    lightingShader.setVec3("pointLights[3].specular", glm::vec3(1.0f, 1.0f, 1.0f));
    lightingShader.setFloat("pointLights[3].constant", 1.0f);
    lightingShader.setFloat("pointLights[3].linear", 0.09f);
    lightingShader.setFloat("pointLights[3].quadratic", 0.032f);

    // 8. 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 计算帧时间差
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 处理输入
        processInput(window);

        // 清空缓冲区
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 激活着色器
        lightingShader.use();

        // 投影矩阵
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 1000.0f);
        lightingShader.setMat4("projection", projection);

        // 视图矩阵（根据视点模式切换）
        glm::mat4 view = glm::mat4(1.0f);
        glm::vec3 viewPos;
        glm::mat4 modelMat = glm::mat4(1.0f);

        if (currentViewMode == ViewMode::MODEL_CENTERED) {
            // 计算模型中心模式的视点位置
            glm::vec3 cameraPos;
            cameraPos.x = modelCenter.x + mc_ModelOffset.x + mc_Distance * cos(glm::radians(mc_Pitch)) * cos(glm::radians(mc_Yaw));
            cameraPos.y = modelCenter.y + mc_ModelOffset.y + mc_Distance * sin(glm::radians(mc_Pitch));
            cameraPos.z = modelCenter.z + mc_ModelOffset.z + mc_Distance * cos(glm::radians(mc_Pitch)) * sin(glm::radians(mc_Yaw));
            viewPos = cameraPos;

            // 看向模型中心
            view = glm::lookAt(cameraPos, modelCenter + mc_ModelOffset, glm::vec3(0.0f, 1.0f, 0.0f));
            // 模型矩阵（平移偏移）
            modelMat = glm::translate(glm::mat4(1.0f), mc_ModelOffset);
        }
        else if (currentViewMode == ViewMode::VIEWPOINT_CENTERED) {
            // 视点中心模式（第一人称）
            view = glm::lookAt(vc_CameraPos, vc_CameraPos + vc_CameraFront, vc_CameraUp);
            viewPos = vc_CameraPos;
            // 模型矩阵（无额外偏移）
            modelMat = glm::mat4(1.0f);
        }

        // 设置视图矩阵和视点位置（setVec3传入glm::vec3对象）
        lightingShader.setMat4("view", view);
        lightingShader.setVec3("viewPos", viewPos);
        lightingShader.setMat4("model", modelMat);

        // 绘制模型
        model->Draw(lightingShader);

        // 交换缓冲区并轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 释放资源
    delete model;
    glfwTerminate();
    return 0;
}