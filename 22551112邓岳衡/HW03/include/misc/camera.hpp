#pragma once

#include "learn_vulkan.hpp"
#include "glfw_wrapper.hpp"

#include <algorithm>

namespace gameworld
{
    class Camera
    {
    public:
        virtual ~Camera() = default;
        virtual void processInput(GLFWwindow *window, float deltaTime) = 0;
        virtual glm::mat4 getViewMatrix() const = 0;
        virtual glm::vec3 getPosition() const = 0;
    };

    class FreeCamera final : public Camera
    {
    public:
        void processInput(GLFWwindow *window, float deltaTime) override
        {
            const float velocity = speed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                pos += front * velocity;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                pos -= front * velocity;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                pos -= glm::normalize(glm::cross(front, up)) * velocity;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                pos += glm::normalize(glm::cross(front, up)) * velocity;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                pos += up * velocity;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                pos -= up * velocity;

            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                double xpos = 0.0;
                double ypos = 0.0;
                glfwGetCursorPos(window, &xpos, &ypos);

                if (firstMouse)
                {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                const float xoffset = static_cast<float>(xpos - lastX) * sensitivity;
                const float yoffset = static_cast<float>(lastY - ypos) * sensitivity;
                lastX = xpos;
                lastY = ypos;

                yaw += xoffset;
                pitch = std::clamp(pitch + yoffset, -89.0f, 89.0f);
                updateFront();
            }
            else
            {
                firstMouse = true;
            }
        }

        glm::mat4 getViewMatrix() const override
        {
            return glm::lookAt(pos, pos + front, up);
        }

        glm::vec3 getPosition() const override { return pos; }

        void setPosition(const glm::vec3 &position)
        {
            pos = position;
        }

        void setYawPitch(float newYaw, float newPitch)
        {
            yaw = newYaw;
            pitch = std::clamp(newPitch, -89.0f, 89.0f);
            updateFront();
        }

        void setMovementSpeed(float newSpeed) { speed = newSpeed; }
        void setSensitivity(float newSensitivity) { sensitivity = newSensitivity; }

    private:
        void updateFront()
        {
            glm::vec3 direction;
            direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            direction.y = sin(glm::radians(pitch));
            direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(direction);
        }

        glm::vec3 pos = glm::vec3(0.0f, 5.0f, 15.0f);
        glm::vec3 front = glm::vec3(0.0f, -0.3f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        float yaw = -90.0f;
        float pitch = -20.0f;
        float speed = 5.0f;
        float sensitivity = 0.1f;
        bool firstMouse = true;
        double lastX = 0.0;
        double lastY = 0.0;
    };

    class OrbitCamera final : public Camera
    {
    public:
        OrbitCamera(glm::vec3 targetPoint = glm::vec3(0.0f), float initialDistance = 15.0f)
            : target(targetPoint), distance(std::max(initialDistance, 0.1f))
        {
            recalculatePosition();
        }

        void processInput(GLFWwindow *window, float deltaTime) override
        {
            handleMouseDrag(window);
            handleZoom(window, deltaTime);
        }

        glm::mat4 getViewMatrix() const override
        {
            return glm::lookAt(position, target, viewUp);
        }

        glm::vec3 getPosition() const override { return position; }

        void setTarget(const glm::vec3 &newTarget)
        {
            target = newTarget;
            recalculatePosition();
        }

        glm::vec3 getTarget() const { return target; }

        void setDistanceLimits(float minDist, float maxDist)
        {
            minDistance = std::max(0.1f, minDist);
            maxDistance = std::max(minDistance, maxDist);
            distance = std::clamp(distance, minDistance, maxDistance);
            recalculatePosition();
        }

        void setRotationSensitivity(float value) { rotationSensitivity = value; }
        void setZoomSpeed(float value) { zoomSpeed = value; }

    private:
        void handleMouseDrag(GLFWwindow *window)
        {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                double xpos = 0.0;
                double ypos = 0.0;
                glfwGetCursorPos(window, &xpos, &ypos);

                if (firstMouse)
                {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                const float xoffset = static_cast<float>(xpos - lastX) * rotationSensitivity;
                const float yoffset = static_cast<float>(lastY - ypos) * rotationSensitivity;
                lastX = xpos;
                lastY = ypos;

                yaw += xoffset;
                pitch = std::clamp(pitch + yoffset, -89.0f, 89.0f);
                recalculatePosition();
            }
            else
            {
                firstMouse = true;
            }
        }

        void handleZoom(GLFWwindow *window, float deltaTime)
        {
            (void)window;
            (void)deltaTime;
            const double scrollOffset = GLFWWrapper::consumeScrollYOffset();
            if (scrollOffset == 0.0)
                return;

            distance = std::clamp(
                distance - static_cast<float>(scrollOffset) * zoomSpeed,
                minDistance,
                maxDistance);
            recalculatePosition();
        }

        void recalculatePosition()
        {
            const float yawRad = glm::radians(yaw);
            const float pitchRad = glm::radians(pitch);
            glm::vec3 offset;
            offset.x = distance * cos(pitchRad) * cos(yawRad);
            offset.y = distance * sin(pitchRad);
            offset.z = distance * cos(pitchRad) * sin(yawRad);
            position = target + offset;

            const glm::vec3 front = glm::normalize(target - position);
            const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::cross(front, worldUp);
            const float rightLength = glm::length(right);
            if (rightLength < 1e-5f)
            {
                viewUp = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            else
            {
                right /= rightLength;
                viewUp = glm::normalize(glm::cross(right, front));
            }
        }

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 target = glm::vec3(0.0f);
        glm::vec3 viewUp = glm::vec3(0.0f, 1.0f, 0.0f);
        float distance = 15.0f;
        float minDistance = 2.0f;
        float maxDistance = 80.0f;
        float yaw = -90.0f;
        float pitch = -20.0f;
        float rotationSensitivity = 0.1f;
        float zoomSpeed = 25.0f;
        bool firstMouse = true;
        double lastX = 0.0;
        double lastY = 0.0;
    };
} // namespace gameworld
