#include "glMotion.h"
#include "camera.h"

extern Camera camera;       // NOT GOOD, but necessary
extern float lastX;
extern float lastY;
extern bool leftMouseButtonPressed;
extern bool rightMouseButtonPressed;
extern float deltaTime;
extern float lastFrame;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
	if(window) return;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.Move(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.Move(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.Move(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.Move(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	// window useless here
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

	if(leftMouseButtonPressed) {
		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
		camera.Rotate(xoffset, yoffset);
		lastX = xpos;
    	lastY = ypos;
	}
    else if(rightMouseButtonPressed) {
        float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
        // move around (0, 0, 0)
        auto center = glm::vec3(0.0f, 0.0f, 0.0f);
        float radius = glm::distance(camera.Position, center);
        camera.Orbit(xoffset, yoffset, center, radius);
        lastX = xpos;
    	lastY = ypos;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseButtonPressed = true;
			double lx, ly;
            glfwGetCursorPos(window, &lx, &ly);
			lastX = (float)lx;
			lastY = (float)ly;
        } else if (action == GLFW_RELEASE) {
            leftMouseButtonPressed = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightMouseButtonPressed = true;
            double lx, ly;
            glfwGetCursorPos(window, &lx, &ly);
			lastX = (float)lx;
			lastY = (float)ly;
        } else {
            rightMouseButtonPressed = false;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// window and xoffset are useless here
    camera.Zoom(static_cast<float>(yoffset));
}
