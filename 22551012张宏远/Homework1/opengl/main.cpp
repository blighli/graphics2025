// 22551012
// 张宏远

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// 使用 GLM 做矩阵变换
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// 顶点着色器源码：增加了一个 transform 矩阵
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(aPos, 1.0);
}
)";

// 片段着色器源码
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0); // 橙色
}
)";

int main()
{
    // 初始化 GLFW
    if (!glfwInit())
    {
        std::cout << "GLFW 初始化失败" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);              // OpenGL 主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);              // OpenGL 次版本号
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 使用核心模式

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);        // MacOS 需要这行
#endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "Rotating Triangle", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "创建窗口失败" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化 GLAD（在有了 OpenGL 上下文之后）
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "GLAD 初始化失败" << std::endl;
        glfwTerminate();
        return -1;
    }

    // ================== 编译着色器 ==================

    // 顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // 可选：检查编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "顶点着色器编译失败:\n" << infoLog << std::endl;
    }

    // 片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "片段着色器编译失败:\n" << infoLog << std::endl;
    }

    // 链接着色器程序
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "着色器程序链接失败:\n" << infoLog << std::endl;
    }

    // 链接完成后删除独立着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // ================== 顶点数据 ==================

    // 三角形顶点数据（NDC 坐标）
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // 左下
         0.5f, -0.5f, 0.0f,  // 右下
         0.0f,  0.5f, 0.0f   // 上
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 绑定 VAO
    glBindVertexArray(VAO);

    // 绑定 VBO，并把顶点数据复制到缓冲中
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 告诉 OpenGL 顶点格式：
    // 位置属性，location = 0，对应着色器中的 aPos
    glVertexAttribPointer(
        0,                  // 顶点属性位置（layout(location = 0)）
        3,                  // 每个顶点由 3 个 float 组成 (x, y, z)
        GL_FLOAT,           // 数据类型
        GL_FALSE,           // 是否归一化
        3 * sizeof(float),  // 步长（每个顶点占用的字节数）
        (void*)0            // 在缓冲区起始位置的偏移量
    );
    glEnableVertexAttribArray(0);

    // 解绑（可选）
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 获取 transform uniform 的位置（只需获取一次）
    glUseProgram(shaderProgram);
    int transformLoc = glGetUniformLocation(shaderProgram, "transform");

    // ================== 渲染循环 ==================
    while (!glfwWindowShouldClose(window))
    {
        // 处理输入
        processInput(window);

        // 清空颜色缓冲
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用着色器程序
        glUseProgram(shaderProgram);

        // 创建一个单位矩阵
        glm::mat4 transform = glm::mat4(1.0f);

        // 让三角形围绕 Z 轴旋转（以时间为角度参数）
        float angle = (float)glfwGetTime(); // 随时间累积
        transform = glm::rotate(transform, angle, glm::vec3(0.0f, 0.0f, 1.0f));

        // 把矩阵传给着色器
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        // 绘制三角形
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 交换前后缓冲并处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ================== 资源释放 ==================
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// 处理输入：按 ESC 退出
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// 窗口大小变化时的回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 让视口和窗口大小保持一致
    glViewport(0, 0, width, height);
}
