#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "camera.h"

// ================= 工具函数 =================
std::string loadFile(const char* path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint compileShader(const char* vs, const char* fs) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// ================= 全局相机 =================
Camera camera;
bool firstMouse = true;
double lastX, lastY;

void mouseCallback(GLFWwindow*, double x, double y) {
    if (firstMouse) {
        lastX = x; lastY = y;
        firstMouse = false;
    }
    camera.processMouse(x - lastX, lastY - y);
    lastX = x;
    lastY = y;
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(1280, 800, "Black Hole", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);

    gladLoadGL();

    // ================= 全屏三角形 =================
    float quad[] = {
        -1,-1,  1,-1,  1,1,
        -1,-1,  1,1,  -1,1
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    // ================= 编译 shader =================
    auto vs = loadFile("Shaders/fullscreen.vert");
    auto fs = loadFile("Shaders/blackhole.frag");
    GLuint program = compileShader(vs.c_str(), fs.c_str());

    // ================= 加载 HDR 星空 =================
    stbi_set_flip_vertically_on_load(true);
    int w, h, n;
    float* data = stbi_loadf("E:/OpenGLLearning/OpenGLHW02/Resources/SpaceStars01 _2K.hdr", &w, &h, &n, 0);
    if (!data) {
        std::cout << "Failed to load HDR sky\n";
        return -1;
    }

    GLuint hdrTex;
    glGenTextures(1, &hdrTex);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(data);

    camera.position = glm::vec3(0.0f, 1.2f, 7.5f);

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float time = glfwGetTime();
        float dt = time - lastTime;
        lastTime = time;

        if (glfwGetKey(window, GLFW_KEY_W)) camera.processKeyboard(GLFW_KEY_W, dt);
        if (glfwGetKey(window, GLFW_KEY_S)) camera.processKeyboard(GLFW_KEY_S, dt);
        if (glfwGetKey(window, GLFW_KEY_A)) camera.processKeyboard(GLFW_KEY_A, dt);
        if (glfwGetKey(window, GLFW_KEY_D)) camera.processKeyboard(GLFW_KEY_D, dt);

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        glUniform3fv(glGetUniformLocation(program, "camPos"), 1, &camera.position[0]);
        glm::mat3 camRot = camera.getRotation();
        glUniformMatrix3fv(glGetUniformLocation(program, "camRot"), 1, GL_FALSE, &camRot[0][0]);
        glUniform1f(glGetUniformLocation(program, "spin"), 0.9f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTex);
        glUniform1i(glGetUniformLocation(program, "hdrSky"), 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
