#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace glm;

extern const float PI;

enum Camera_Movement
{
	FORWARD, BACKWARD, LEFT, RIGHT
};

class Camera {
private:
	glm::vec3 m_Pos;
	glm::vec3 m_Front;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;

	//euler angle
	float m_Pitch;
	float m_Yaw;

	float m_Speed;
	float m_MouseSensitivity;

	float m_Fov;

	void updateCameraVector();
public:
	Camera(const glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f), const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f));
	~Camera() {}

	glm::mat4 GetViewMatrix() const;

	inline float GetFov() const { return m_Fov; }
	inline glm::vec3 GetPosition() const { return m_Pos; }
	inline glm::vec3 GetFront() const { return m_Front; }
	inline glm::vec3 GetUp() const { return m_Up; }
	inline glm::vec3 GetRight() const { return m_Right; }

	void ProcessKeyboard(const Camera_Movement dir, const float delatTime);
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
	void ProcessMouseScroll(float xoffset, float yoffset);

	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
};

class newCamera {
public:
	newCamera(
		float vfov, 
		float aspect = 1.0f,
		vec3 lookfrom = vec3(0), vec3 lookat = vec3(0, 0, -1), vec3 vup = vec3(0, 1, 0)
	) :vfov(vfov), aspect(aspect), origin(lookfrom), lookat(lookat),vup(vup)
	{
		update();
	}


public:
	vec3 origin;
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;

	bool constrainPitch = true;

	void ProcessKeyboard(const Camera_Movement dir, const float delatTime);
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
	void ProcessMouseScroll(float xoffset, float yoffset);

	inline float GetFov() { return vfov; }
	inline mat4 GetViewMatrix() { return lookAt(origin, lookat, normalize(vertical)); }

private:
	float vfov;
	vec3 lookat;
	vec3 vup;
	float aspect;

	vec3 right;
	vec3 up;
	vec3 front;

	void update();
};