#include "glfw_wrapper.hpp"
#include "vulkan_manager.hpp"

#include <spdlog/spdlog.h>

using namespace vulkan;
bool GLFWWrapper::initialized = false;
GLFWwindow *GLFWWrapper::window = nullptr;
GLFWmonitor *GLFWWrapper::monitor = nullptr;

bool GLFWWrapper::initGlfw(int width, int height, const char *title)
{
    if (initialized)
    {
        spdlog::warn("GLFW is already initialized.");
        return true;
    }

    if (!glfwInit())
    {
        spdlog::error("Failed to initialize GLFW.");
        return false;
    }
    initialized = true;

    // Set GLFW to not create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    spdlog::info("GLFW initialized successfully.");

    // Create a windowed mode window
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        spdlog::error("Failed to create GLFW window.");
        glfwTerminate();
        initialized = false;
        return false;
    }
    spdlog::info("GLFW window created successfully.");

    // Get the primary monitor
    monitor = glfwGetPrimaryMonitor();
    return true;
}

bool GLFWWrapper::terminateGlfw()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return false;
    }

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
        spdlog::info("GLFW window destroyed.");
    }

    glfwTerminate();
    initialized = false;
    spdlog::info("GLFW terminated successfully.");
    return true;
}

void GLFWWrapper::terminateWindowWithVulkan()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return;
    }

    // Wait for device to be idle before destroying resources
    if (VulkanManager::getManager().waitIdleDevice() != VK_SUCCESS)
    {
        spdlog::error("Failed to wait for device to be idle before terminating.");
    }

    // terminate glfw
    terminateGlfw();
}

void GLFWWrapper::pollEvents()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return;
    }
    glfwPollEvents();
}

bool GLFWWrapper::shouldClose()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return true;
    }
    return glfwWindowShouldClose(window);
}

void GLFWWrapper::fullscreen()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return;
    }
    if (!monitor)
    {
        spdlog::error("No primary monitor found.");
        return;
    }

    // get the video mode of the monitor evrery time
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode)
    {
        spdlog::error("Failed to get video mode of the primary monitor.");
        return;
    }

    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    spdlog::info("Switched to fullscreen mode.");
}

void GLFWWrapper::windowed(int width, int height)
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return;
    }

    // Center the window on the primary monitor
    if (monitor)
    {
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        if (mode)
        {
            int xpos = (mode->width - width) / 2;
            int ypos = (mode->height - height) / 2;
            glfwSetWindowPos(window, xpos, ypos);
        }
    }

    glfwSetWindowMonitor(window, nullptr, 100, 100, width, height, 0);
    spdlog::info("Switched to windowed mode.");
}

void GLFWWrapper::setWindowTitle(const char *title)
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return;
    }
    glfwSetWindowTitle(window, title);
}

void GLFWWrapper::waitEvents()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return;
    }
    glfwWaitEvents();
}

bool GLFWWrapper::isMinimized()
{
    if (!initialized)
    {
        spdlog::warn("GLFW is not initialized.");
        return false;
    }
    
    // Check if the window is iconified (minimized)
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE;
}

// Initialize Vulkan instance and device using VulkanManager
bool GLFWWrapper::initVulkan()
{
    if (!initialized)
    {
        spdlog::error("GLFW is not initialized.");
        return false;
    }

    // Add Vulkan instance extensions required by GLFW
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensions)
    {
        spdlog::error("Failed to get required GLFW extensions.");
        glfwTerminate();
        initialized = false;
        return false;
    }
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        // spdlog::info("GLFW required extension: {}", glfwExtensions[i]);
        VulkanManager::getManager().addInstanceExtension(glfwExtensions[i]);
    }

    // Add win32 and swapchain extension
    VulkanManager::getManager().addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
    VulkanManager::getManager().addInstanceExtension("VK_KHR_win32_surface");
    VulkanManager::getManager().addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    VulkanManager::getManager().useLatestVulkanApiVersion();
    if (VkResult result = VulkanManager::getManager().createInstance(); result != VK_SUCCESS)
    {
        spdlog::error("Failed to create Vulkan instance: {}", static_cast<int32_t>(result));
        terminateGlfw();
        return false;
    }

    // Create Vulkan surface for the GLFW window
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(VulkanManager::getManager().getInstance(), window, nullptr, &surface) != VK_SUCCESS)
    {
        spdlog::error("Failed to create Vulkan surface.");
        terminateGlfw();
        return false;
    }
    VulkanManager::getManager().setSurface(surface);

    // Create Vulkan physical device and logical device
    if (VulkanManager::getManager().getAvailablePhysicalDevices() ||
        VulkanManager::getManager().selectPhysicalDevice() ||
        VulkanManager::getManager().createDevice())
    {
        spdlog::error("Failed to create Vulkan physical or logical device.");
        terminateGlfw();
        return false;
    }

    // Create swapchain
    if (VulkanManager::getManager().createSwapchain())
    {
        spdlog::error("Failed to create Vulkan swapchain.");
        terminateGlfw();
        return false;
    }
    return true;
}

bool GLFWWrapper::initWindowWithVulkan(int width, int height, const char *title)
{
    // First initialize GLFW
    if (!initGlfw(width, height, title))
    {
        spdlog::error("Failed to initialize GLFW.");
        return false;
    }

    // Then initialize Vulkan
    if (!initVulkan())
    {
        spdlog::error("Failed to initialize Vulkan.");
        terminateGlfw();
        return false;
    }

    return true;
}