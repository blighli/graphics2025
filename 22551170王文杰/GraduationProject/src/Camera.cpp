#include "Camera.h"

Camera::Camera(const glm::vec3 pos, const glm::vec3 worldUp)
	:m_Pos(pos), m_WorldUp(worldUp), m_Speed(2.5f),
	m_Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	m_Pitch(0.0f), m_Yaw(-90.0f), m_MouseSensitivity(0.01f),
	m_Fov(60.0f)
{
	updateCameraVector();
}

void Camera::updateCameraVector()
{
	glm::vec3 front;
	front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	front.y = sin(glm::radians(m_Pitch));
	front.z = cos(glm::radians(m_Pitch)) * sin(glm::radians(m_Yaw));

	m_Front = glm::normalize(front);
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));

	vec3 u, v, w;

	auto theta = radians(m_Fov);
	float half_height = tan(theta / 2);
	float half_width = half_height;
	w = -m_Front;
	u = m_Right;
	v = m_Up;

	lower_left_corner = m_Pos - half_width * u - half_height * v - w;

	horizontal = 2 * half_width * u;
	vertical = 2 * half_height * v;
}
glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(m_Pos, m_Pos + m_Front, m_Up);
}


void Camera::ProcessKeyboard(const Camera_Movement dir, const float delatTime)
{
	float speed = m_Speed * delatTime;
	if (dir == Camera_Movement::FORWARD)
		m_Pos += m_Front * speed;
	if (dir == Camera_Movement::BACKWARD)
		m_Pos -= m_Front * speed;
	if (dir == Camera_Movement::LEFT)
		m_Pos -= m_Right * speed;
	if (dir == Camera_Movement::RIGHT)
		m_Pos += m_Right * speed;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	xoffset *= m_MouseSensitivity;
	yoffset *= m_MouseSensitivity;

	m_Yaw += xoffset;
	m_Pitch += yoffset;

	//¿ØÖÆÊÓ½Ç·¶Î§
	if (constrainPitch)
	{
		if (m_Pitch > 89.0f)
			m_Pitch = 89.0f;
		if (m_Pitch < -89.0f)
			m_Pitch = -89.0f;
	}
	updateCameraVector();
}

void Camera::ProcessMouseScroll(float xoffset, float yoffset)
{
	if (m_Fov >= 1.0f && m_Fov <= 100.0f)
		m_Fov -= yoffset;
	if (m_Fov < 1.0f)
		m_Fov = 1.0f;
	if (m_Fov > 100.0f)
		m_Fov = 100.0f;
}

void newCamera::ProcessKeyboard(const Camera_Movement dir, const float delatTime)
{
	float speed = 2.0 * delatTime;
	if (dir == Camera_Movement::FORWARD) 
	{
		origin += front * speed;
		lookat += front * speed;
	}
	if (dir == Camera_Movement::BACKWARD)
	{
		origin -= front * speed;
		lookat -= front * speed;
	}
	if (dir == Camera_Movement::LEFT)
	{
		origin -= right * speed;
		lookat -= right * speed;
	}
	if (dir == Camera_Movement::RIGHT)
	{
		origin += right * speed;
		lookat += right * speed;
	}
	update();
}

void newCamera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	xoffset *= 0.001f;
	yoffset *= 0.001f;
	
	if(constrainPitch)
		lookat = lookat + vec3(xoffset, yoffset, 0);
	update();
}

void newCamera::ProcessMouseScroll(float xoffset, float yoffset)
{
	if (vfov >= 1.0f && vfov <= 100.0f)
		vfov -= yoffset;
	if (vfov < 1.0f)
		vfov = 1.0f;
	if (vfov > 100.0f)
		vfov = 100.0f;

	update();
}

void newCamera::update()
{
	vec3 u, v, w;

	auto theta = radians(vfov);
	float half_height = tan(theta / 2);
	float half_width = aspect * half_height;
	w = normalize(origin - lookat);
	u = normalize(cross(vup, w));
	v = cross(w, u);

	lower_left_corner = origin - half_width * u - half_height * v - w;

	horizontal = 2 * half_width * u;
	vertical = 2 * half_height * v;

	front = normalize(lookat - origin);
	right = normalize(cross(vup, -front));
	up = normalize(cross(right, front));
}
