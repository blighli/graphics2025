#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class GLFWWrapper
{
public:
    static bool initWindowWithVulkan(int width = 800, int height = 600, const char *title = "LearnVulkan");
    static void terminateWindowWithVulkan();
    static bool shouldClose();
    static void pollEvents();
    static void waitEvents();
    static void fullscreen();
    static void windowed(int width, int height);
    static void setWindowTitle(const char *title);
    static bool isMinimized();
    static double consumeScrollYOffset();
    
    static bool isInitialized() { return initialized; }
    static GLFWwindow *getWindow() { return window; }
    
private:
    static bool initGlfw(int width = 800, int height = 600, const char *title = "LearnVulkan");
    static bool terminateGlfw();
    static bool initVulkan();
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    static bool initialized;
    static GLFWwindow *window;
    static GLFWmonitor *monitor;
    static double scrollYOffset;
};
