#pragma once

#include "Test.h"
#include "../Scene.h"
#include "../Shape.h"

#include<memory>

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

extern float deltaTime;
extern float lastFrame;

extern Material RED;
extern Material GREEN;
extern Material BLUE;
extern Material YELLOW;
extern Material PURPLE;
extern Material CYAN;
extern Material WHITE;
extern Material GREY;
extern Material WHITE_MIRROR;

extern std::vector<glm::vec3> Left;
extern std::vector<glm::vec3> Right;
extern std::vector<glm::vec3> Up;
extern std::vector<glm::vec3> Down;
extern std::vector<glm::vec3> Back;
extern std::vector<glm::vec3> LightUp;

extern const float PI;

namespace test {
	class TestRenderRealTime : public Test
	{
	public:
		TestRenderRealTime();
		~TestRenderRealTime();

		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		std::unique_ptr<Shader> m_PathTracingShader;
		std::unique_ptr<Scene> m_Scene;

		std::vector<std::unique_ptr<Plane>> planes;
		std::vector<std::unique_ptr<Cube>> cubes;
		std::vector<std::unique_ptr<Sphere>> spheres;

		int spp = 8;
		int frameCounter = 0;
	};
}