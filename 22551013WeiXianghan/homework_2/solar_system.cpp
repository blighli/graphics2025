#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "learnopengl/shader.h"
#include "learnopengl/camera.h"
#include "learnopengl/model.h"
#include "learnopengl/filesystem.h"

#include <iostream>


// ======================= settings =======================
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;

// ======================= camera =========================
Camera camera(glm::vec3(0.0f, 50.0f, 20.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// ======================= timing =========================
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ======================= lighting =======================
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

// ======================= 星体基本数据 =======================
const float R_earthOrbit = 10.0f; // 半径
const float W_earthOrbit = 0.1f; // 角速度
const float R_moonOrbit = 1.0f;
const float W_moonOrbit = 10.0f;
const float Scale_earth = 1.0f; // 缩放
const float Scale_moon = 0.15f;
const float Scale_sun = 2.0f;

// ======================= callbacks ======================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

unsigned int loadTexture(const char* path);


// ======================= 坐标轴的绘制 =======================
float axisVertices[] = {
    // positions         // colors
    -10.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f,  // X轴起点 (红色)
    100.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // X轴终点 (红色)

    0.0f, -10.0f, 0.0f,    0.0f, 1.0f, 0.0f,  // Y轴起点 (绿色)
    0.0f, 100.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Y轴终点 (绿色)

    0.0f, 0.0f, -10.0f,    0.0f, 0.0f, 1.0f,  // Z轴起点 (蓝色)
    0.0f, 0.0f, 100.0f,   0.0f, 0.0f, 1.0f   // Z轴终点 (蓝色)
};

unsigned int axisVAO, axisVBO;

void setupAxis()
{
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);

    glBindVertexArray(axisVAO);

    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void drawAxis(Shader& shader, const glm::mat4& projection, const glm::mat4& view)
{
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);

    glBindVertexArray(axisVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}



// ======================= 地球公转轨道的绘制 =======================
const int orbitSegments = 100; // 圆的分段数
float orbitVertices[orbitSegments * 6]; // 每个点有位置和颜色

unsigned int orbitVAO, orbitVBO;

void setupOrbit()
{
    for (int i = 0; i < orbitSegments; ++i)
    {
        float theta = 2.0f * 3.1415926f * float(i) / float(orbitSegments);
        float nextTheta = 2.0f * 3.1415926f * float(i + 1) / float(orbitSegments);

        // 当前点
        orbitVertices[i * 6 + 0] = R_earthOrbit * cos(theta); // x
        orbitVertices[i * 6 + 1] = 0.0f;             // y
        orbitVertices[i * 6 + 2] = R_earthOrbit * sin(theta); // z
        orbitVertices[i * 6 + 3] = 1.0f;             // r
        orbitVertices[i * 6 + 4] = 1.0f;             // g
        orbitVertices[i * 6 + 5] = 1.0f;             // b

        // 下一点（闭合圆）
        orbitVertices[(i + 1) * 6 + 0] = R_earthOrbit * cos(nextTheta);
        orbitVertices[(i + 1) * 6 + 1] = 0.0f;
        orbitVertices[(i + 1) * 6 + 2] = R_earthOrbit * sin(nextTheta);
        orbitVertices[(i + 1) * 6 + 3] = 1.0f;
        orbitVertices[(i + 1) * 6 + 4] = 1.0f;
        orbitVertices[(i + 1) * 6 + 5] = 1.0f;
    }

    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);

    glBindVertexArray(orbitVAO);

    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(orbitVertices), orbitVertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void drawOrbit(Shader& shader, const glm::mat4& projection, const glm::mat4& view)
{
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);

    glBindVertexArray(orbitVAO);
    glDrawArrays(GL_LINE_LOOP, 0, orbitSegments);
    glBindVertexArray(0);
}



int main()
{
    // glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // shaders
    Shader lightingShader(
        FileSystem::getPath("resources/shaders/light_casters.vs").c_str(),
        FileSystem::getPath("resources/shaders/light_casters.fs").c_str()
    );
    // Shader lightCubeShader(
    //     FileSystem::getPath("resources/shaders/5.2.light_cube.vs").c_str(),
    //     FileSystem::getPath("resources/shaders/5.2.light_cube.fs").c_str()
    // );
    Shader defaultShader(
        FileSystem::getPath("resources/shaders/default.vs").c_str(),
        FileSystem::getPath("resources/shaders/default.fs").c_str()
    );

    // model
    Model sphere(FileSystem::getPath("resources/objects/sphere_move_to_origin.obj"));

    // textures
    unsigned int sunDiffuse  = loadTexture(FileSystem::getPath("resources/textures/sun.jpg").c_str());
    unsigned int earthDiffuse = loadTexture(FileSystem::getPath("resources/textures/earth.jpg").c_str());
    unsigned int moonDiffuse  = loadTexture(FileSystem::getPath("resources/textures/moon.jpg").c_str());


    // unsigned int earthSpecular = 
    // unsigned int moonSpecular  = 

    // shader configuratio
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    // lightingShader.setInt("material.specular", 1);

    // 在程序开始时设置相机指向太阳
    camera.Yaw = -90.0f; 
    camera.Pitch = 0.0f; 
    camera.updateCameraVectors();

    // 初始化坐标轴
    setupAxis();
    // 初始化地球公转轨道
    setupOrbit();

    while (!glfwWindowShouldClose(window))
    {
        // timing
        float currentFrame = (float)glfwGetTime();
        //减缓时间的流逝速度
        // currentFrame *= 0.1f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        //试试正交投影
        // glm::mat4 projection = glm::ortho(
        //     -50.0f, 50.0f,    // 根据太阳系尺寸调整
        //     -50.0f, 50.0f,
        //     0.1f, 200.0f
        // );
        glm::mat4 view = camera.GetViewMatrix();

        drawAxis(defaultShader, projection, view); // 绘制坐标轴
        drawOrbit(defaultShader, projection, view); // 绘制地球公转轨道

        // 设置Lightning Shader的uniforms

        float t = (float)glfwGetTime();

        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);


        lightingShader.setVec3("light.position", lightPos);
        lightingShader.setVec3("viewPos", camera.Position);

        lightingShader.setVec3("light.ambient", 0.6f, 0.6f, 0.6f);
        lightingShader.setVec3("light.diffuse", 2.0f, 2.0f, 2.0f);
        lightingShader.setVec3("light.specular", 2.0f, 2.0f, 2.0f);
        lightingShader.setFloat("light.constant", 1.0f);
        lightingShader.setFloat("light.linear", 0.02f);      // 线性衰减
        lightingShader.setFloat("light.quadratic", 0.08f);  // 二次衰减


        
        // ==========================
        // 1) 画太阳
        // ==========================
        
        // set model
        glm::mat4 sunModel = glm::mat4(1.0f);
        //sunModel = glm::translate(sunModel, lightPos);
        sunModel = glm::scale(sunModel, glm::vec3(Scale_sun));
        //加个缓慢的自转
        float sunSelfRotateAngleSpeed = 0.05f; // 太阳自转角速度
        sunModel = glm::rotate(sunModel, t * sunSelfRotateAngleSpeed, glm::vec3(0, 1, 0));
        lightingShader.setMat4("model", sunModel);

        // bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunDiffuse);

        // draw sun
        sphere.Draw(lightingShader);

        // ==========================
        // 2) 画地球
        // ==========================

        // set model
        glm::mat4 earthModel = glm::mat4(1.0f);
        // earthModel = glm::rotate(earthModel, t * 0.5f, glm::vec3(0, 1, 0));
        // earthModel = glm::translate(earthModel, glm::vec3(3.0f, 0.0f, 0.0f));
        // 改为使用三角函数计算每一时刻的位置，仅平移
        float earthX = R_earthOrbit * cos(t * W_earthOrbit);
        float earthZ = R_earthOrbit * sin(t * W_earthOrbit);
        earthModel = glm::translate(earthModel, glm::vec3(earthX, 0.0f, earthZ));
        earthModel = glm::scale(earthModel, glm::vec3(Scale_earth));
        //另外再加个地球自转
        float earthSelfRotateAngleSpeed = 2.0f; // 地球自转角速度
        earthModel = glm::rotate(earthModel, t * earthSelfRotateAngleSpeed, glm::vec3(0, 1, 0));
        lightingShader.setMat4("model", earthModel);

        // bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earthDiffuse);
        // glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, earthSpecular);

        // draw earth
        sphere.Draw(lightingShader);

        // ==========================
        // 3) 画月球
        // ==========================

        // set model
        glm::mat4 moonModel = glm::mat4(1.0f);
        // 改为使用三角函数计算相对地球的位置，仅平移
        float moonX = R_moonOrbit * cos(t * W_moonOrbit);
        float moonZ = R_moonOrbit * sin(t * W_moonOrbit);
        moonModel = glm::translate(moonModel, glm::vec3(earthX + moonX, 0.0f, earthZ + moonZ));
        moonModel = glm::scale(moonModel, glm::vec3(Scale_moon));
        lightingShader.setMat4("model", moonModel);

        // bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, moonDiffuse);
        // glActiveTexture(GL_TEXTURE1);
        // glBindTexture(GL_TEXTURE_2D, moonSpecular);

        // draw moon
        sphere.Draw(lightingShader);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ======================= input & callbacks =======================
float camera_move_speed = 5.0f;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * camera_move_speed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * camera_move_speed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * camera_move_speed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime * camera_move_speed);

    // 在 processInput 函数中添加按键 C 的处理逻辑
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        camera.Position = glm::vec3(0.0f, 100.0f, 0.0f); // 设置相机位置到太阳系正上方
        camera.Yaw = 0; // 确保相机朝向原点
        camera.Pitch = -90.0f; // 垂直向下看
        camera.updateCameraVectors(); // 强制更新相机向量
    }

    // 在 processInput 函数中添加 J 和 K 键的处理逻辑
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        camera.Position.y += deltaTime * camera_move_speed; // 向上移动
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        camera.Position.y -= deltaTime * camera_move_speed; // 向下移动

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) // 按下 M 键切换鼠标模式
    {
        static bool cursorDisabled = true;
        if (cursorDisabled)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // 恢复鼠标
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 锁定鼠标
        }
        cursorDisabled = !cursorDisabled;
    }

    // 按下O键 相机置于原点 垂直向下
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
        camera.Position = glm::vec3(0.0f, 0.0f, 0.0f); // 设置相机位置到原点
        camera.Yaw = -90.0f; // 确保相机朝向负Z轴
        camera.Pitch = -89.0f; // 垂直向下看...吗？
        camera.ProcessMouseMovement(0.0f, 0.0f); // 更新相机方向
    }


}

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}

// ======================= texture loader (同 light_casters_point.cpp) =======================

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
            format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << "\n";
        stbi_image_free(data);
    }

    return textureID;
}