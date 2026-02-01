
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

static unsigned int compileShader(unsigned int type, const char* source)
{
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "着色器编译错误：" << infoLog << std::endl;
	}
	return shader;
}

int main()
{
	// 初始化 GLFW
	if (!glfwInit())
	{
		std::cerr << "初始化 GLFW 失败" << std::endl;
		return -1;
	}

	// 请求 OpenGL 4.0 core
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Windows only — no additional platform-specific hints required here

	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Triangle", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "创建 GLFW 窗口失败" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// 初始化 GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "初始化 GLAD 失败" << std::endl;
		return -1;
	}

	// 顶点着色器（传递顶点坐标到片段着色器以计算渐变颜色）
	const char* vertexShaderSource =
		"#version 400 core\n"
		"layout(location = 0) in vec3 aPos;\n"
		"out vec3 vPos;\n"
		"void main()\n"
		"{\n"
		"    vPos = aPos;\n"
		"    gl_Position = vec4(aPos, 1.0);\n"
		"}\n";

	// 片段着色器（根据插值后的顶点坐标计算颜色）
	const char* fragmentShaderSource =
		"#version 400 core\n"
		"in vec3 vPos;\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"    // 将坐标从大约 [-0.5,0.5] 映射到 [0,1]，作为颜色分量\n"
		"    vec3 color = vPos * 0.5 + vec3(0.5);\n"
		"    FragColor = vec4(color, 1.0);\n"
		"}\n";

	unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cerr << "程序链接错误：" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// 三角形顶点数据
	float vertices[] = {
		 0.0f,  0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// 渲染循环
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

