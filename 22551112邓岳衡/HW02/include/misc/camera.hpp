#pragma once

#include "learn_vulkan.hpp"
#include "glfw_wrapper.hpp"

namespace gameworld
{
    class Camera
    {
    public:
        void processInput(GLFWwindow *window, float deltaTime)
        {
            // key movement
            float velocity = speed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                pos += front * velocity;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                pos -= front * velocity;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                pos -= glm::normalize(glm::cross(front, up)) * velocity;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                pos += glm::normalize(glm::cross(front, up)) * velocity;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                pos += up * velocity;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                pos -= up * velocity;

            // mouse movement
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);

                if (firstMouse)
                {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                float xoffset = static_cast<float>(xpos - lastX);
                float yoffset = static_cast<float>(lastY - ypos);
                lastX = xpos;
                lastY = ypos;

                xoffset *= sensitivity;
                yoffset *= sensitivity;

                yaw += xoffset;
                pitch += yoffset;

                if (pitch > 89.0f)
                    pitch = 89.0f;
                if (pitch < -89.0f)
                    pitch = -89.0f;

                glm::vec3 direction;
                direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                direction.y = sin(glm::radians(pitch));
                direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                front = glm::normalize(direction);
            }
            else
            {
                firstMouse = true;
            }
        }

        glm::mat4 getViewMatrix() const
        {
            return glm::lookAt(pos, pos + front, up);
        }

        glm::vec3 getPosition() const { return pos; }

    private:
        glm::vec3 pos = glm::vec3(0.0f, 5.0f, 15.0f);
        glm::vec3 front = glm::vec3(0.0f, -0.3f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        float yaw = -90.0f;
        float pitch = -20.0f;
        float speed = 5.0f;
        float sensitivity = 0.1f;
        bool firstMouse = true;
        double lastX, lastY;
    };
} // namespace gameworld
