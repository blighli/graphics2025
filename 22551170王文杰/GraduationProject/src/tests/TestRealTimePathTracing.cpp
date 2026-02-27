#include "TestRealTimePathTracing.h"

#include "imgui/imgui.h"

#include "../Camera.h"

extern newCamera newcamera;

test::TestRenderRealTime::TestRenderRealTime()
{
	Material LightMaterial({ 1,1,1 });
	LightMaterial.emissive = glm::vec3(1.0f);

	// 着色器
	m_PathTracingShader = std::make_unique<Shader>("res/shaders/RealTime.shader");
	m_PathTracingShader->Bind();
	m_PathTracingShader->SetUniform1i("width", SCR_WIDTH);
	m_PathTracingShader->SetUniform1i("height", SCR_HEIGHT);


	// 物体
	// 盒子
	planes.push_back(std::make_unique<Plane>(Left, glm::vec3(-1, 0, 0), RED));
	planes.push_back(std::make_unique<Plane>(Right, glm::vec3(-1, 0, 0), GREEN));
	planes.push_back(std::make_unique<Plane>(Up, glm::vec3(0, -1, 0), WHITE));
	planes.push_back(std::make_unique<Plane>(Down, glm::vec3(0, -1, 0), WHITE));
	planes.push_back(std::make_unique<Plane>(Back, glm::vec3(0, 0, -1), WHITE_MIRROR));
	planes.push_back(std::make_unique<Plane>(LightUp, glm::vec3(0, -1, 0), LightMaterial));

	// cube
	cubes.push_back(std::make_unique<Cube>(glm::vec3(0.3, -1.5, -5.0), CYAN));
	cubes.push_back(std::make_unique<Cube>(glm::vec3(-1.0, -1.2, -3.1), WHITE, 0.5, 0.8, 0.5, PI / 4.0f));

	// sphere
	//spheres.push_back(std::make_unique<Sphere>(glm::vec3(0, 0.5, -5.5), 0.5, WHITE_MIRROR);
	//spheres.push_back(std::make_unique<Sphere>(glm::vec3(1.5, -1, -6), 0.3, WHITE));

	// 场景
	m_Scene = std::make_unique<Scene>();
	
	for (auto& plane : planes)
		m_Scene->push(plane.get());
	for (auto& cube : cubes)
		m_Scene->push(cube.get());
	for (auto& sphere : spheres)
		m_Scene->push(sphere.get());
}

test::TestRenderRealTime::~TestRenderRealTime()
{
}

void test::TestRenderRealTime::OnUpdate(float deltaTime)
{
}

void test::TestRenderRealTime::OnRender()
{
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	{
		glm::mat4 projection = glm::perspective(glm::radians(newcamera.GetFov()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = newcamera.GetViewMatrix();

		m_PathTracingShader->Bind();
		m_PathTracingShader->SetUniform1i("spp", spp);
		m_PathTracingShader->SetUniformMat4f("projection", projection);
		m_PathTracingShader->SetUniformMat4f("view", view);
		m_PathTracingShader->SetUniform1i("frameCount", frameCounter++);

		m_PathTracingShader->SetUniform3fv("camera.lower_left_corner", newcamera.lower_left_corner);
		m_PathTracingShader->SetUniform3fv("camera.horizontal", newcamera.horizontal);
		m_PathTracingShader->SetUniform3fv("camera.vertical", newcamera.vertical);
		m_PathTracingShader->SetUniform3fv("camera.origin", newcamera.origin);

		// 渲染数据读入帧缓冲区
		m_Scene->Render(*m_PathTracingShader);
	}
}

void test::TestRenderRealTime::OnImGuiRender()
{
	ImGui::SliderInt("spp", &spp, 1, 64);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
}
